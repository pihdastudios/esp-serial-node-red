#include "main.hpp"

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            // ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        // ESP_LOGI(TAG,"connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        // ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        // ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void DHT_task(void *pvParameter)
{
    DHT dht;
    dht.setDHTgpio(DHT_PIN);
    while (1)
    {
        int ret = dht.readDHT();
        if (ret == DHT_OK)
        {
            //Serialize JSON
            cJSON *root = cJSON_CreateObject();
            cJSON_AddNumberToObject(root, "temp", dht.getTemperature());
            cJSON_AddNumberToObject(root, "hum", dht.getHumidity());
            xSemaphoreTake(mutex, portMAX_DELAY);
            std::cout << cJSON_PrintUnformatted(root) << std::endl;
            xSemaphoreGive(mutex);
            cJSON_Delete(root);
            vTaskDelay(2000 / portTICK_RATE_MS);
        }
    }
}

// interrupt service routine, called when the button is pressed
void IRAM_ATTR button_isr_handler(void *pvParameter)
{
    xTaskResumeFromISR(ISR);
}

// task that will react to button clicks
void button_task(void *pvParameter)
{
    while (1)
    {
        vTaskSuspend(nullptr);
        static int64_t last_interrupt_time = 0;
        int64_t interrupt_time = esp_timer_get_time();
        if (interrupt_time - last_interrupt_time > 200000)
        {
            last_interrupt_time = interrupt_time;
            led_status = !led_status;
            gpio_set_level(LED_PIN, led_status);
        }
    }
}

void serial_task(void *pvParameter)
{
    while (1)
    {
        std::string s;
        std::cin >> s;
        if (s == "toggle")
        {
            led_status = !led_status;
            gpio_set_level(LED_PIN, led_status);
        }
    }
}

void app_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Disable buffer
    setvbuf(stdin, nullptr, _IONBF, 0);
    setvbuf(stdout, nullptr, _IONBF, 0);

    gpio_pad_select_gpio(BTN_PIN);
    gpio_pad_select_gpio(LED_PIN);

    // set the correct direction
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    //set pullup mode
    gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_ONLY);

    // enable interrupt on falling (1->0) edge for button pin
    gpio_set_intr_type(BTN_PIN, GPIO_INTR_NEGEDGE);

    // Install the driver’s GPIO ISR handler service, which allows per-pin GPIO interrupt handlers.
    // install ISR service with default configuration
    gpio_install_isr_service(0);

    // attach the interrupt service routine
    gpio_isr_handler_add(BTN_PIN, button_isr_handler, nullptr);

    mutex = xSemaphoreCreateMutex();

    // Enable cin uart
    esp_vfs_dev_uart_use_driver(0);
    uart_driver_install((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM,
                        256, 0, 0, nullptr, 0);

    esp_netif_init();

    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.sta.ssid, EXAMPLE_ESP_WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, EXAMPLE_ESP_WIFI_PASS);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
    vEventGroupDelete(s_wifi_event_group);
    //Create and start stats task
    xTaskCreate(button_task, "button_task", 4096, nullptr, 20, &ISR);
    xTaskCreate(&DHT_task, "DHT_task", 2048, nullptr, 5, nullptr);
    xTaskCreate(&serial_task, "serial_task", 2048, nullptr, 10, nullptr);
}