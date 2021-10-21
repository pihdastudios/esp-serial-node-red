#include "main.hpp"

void DHT_task(void *pvParameter)
{
    DHT dht;
    dht.setDHTgpio(DHT_PIN);
    while (1)
    {
        int ret = dht.readDHT();
        dht.errorHandler(ret);

        std::cout << "====" << std::endl;
        std::cout << "Hum  : " << dht.getHumidity() << std::endl;
        std::cout << "Temp : " << dht.getTemperature() << std::endl;

        // -- wait at least 2 sec before reading again ------------
        // The interval of whole process must be beyond 2 seconds !!
        vTaskDelay(2500 / portTICK_RATE_MS);
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
    bool led_status = false;
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
            std::cout << "Button pressed!!!" << std::endl;
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

    //Create and start stats task

    xTaskCreate(button_task, "button_task", 4096, nullptr, 10, &ISR);

    xTaskCreate(&DHT_task, "DHT_task", 2048, nullptr, 5, nullptr);
}