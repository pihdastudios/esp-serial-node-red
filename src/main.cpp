// #include <stdio.h>
#include <iostream>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_system.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "DHT.hpp"
#define LED_PIN GPIO_NUM_23
#define DHT_PIN GPIO_NUM_4
#define BTN_PIN GPIO_NUM_5

extern "C"
{
    void app_main();
}

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

TaskHandle_t ISR = NULL;

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
        vTaskSuspend(NULL);
        led_status = !led_status;
        gpio_set_level(LED_PIN, led_status);
        std::cout << "Button pressed!!!" << std::endl;
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

    // enable interrupt on falling (1->0) edge for button pin
    gpio_set_intr_type(BTN_PIN, GPIO_INTR_NEGEDGE);

    // Install the driver’s GPIO ISR handler service, which allows per-pin GPIO interrupt handlers.
    // install ISR service with default configuration
    gpio_install_isr_service(0);

    // attach the interrupt service routine
    gpio_isr_handler_add(BTN_PIN, button_isr_handler, NULL);

    //Create and start stats task

    xTaskCreate(&DHT_task, "DHT_task", 2048, NULL, 5, NULL);
    xTaskCreate(button_task, "button_task", 4096, NULL, 10, &ISR);
}