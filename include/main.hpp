#pragma once
#include <iostream>
#include <iomanip>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_vfs_dev.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "mqtt_client.h"

#define MQTT_PUB_TEMP "esp32/dht/temperature"
#define MQTT_PUB_HUM  "esp32/dht/humidity"

// #include "cJSON.h"

#include "DHT.hpp"

#define LED_PIN GPIO_NUM_23
#define DHT_PIN GPIO_NUM_4
#define BTN_PIN GPIO_NUM_5

#define EXAMPLE_ESP_WIFI_SSID "null"
#define EXAMPLE_ESP_WIFI_PASS "027412345678"
#define EXAMPLE_ESP_MAXIMUM_RETRY 5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

TaskHandle_t ISR = nullptr;
static bool led_status = false;

static SemaphoreHandle_t mutex;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data);


static int s_retry_num = 0;

const char TAG[] = "MQTT_EXAMPLE";

std::string ip_address;

void DHT_task(void *pvParameter);
void IRAM_ATTR button_isr_handler(void *pvParameter);
void button_task(void *pvParameter);
void serial_task(void *pvParameter);

extern "C"
{
    void app_main();
}