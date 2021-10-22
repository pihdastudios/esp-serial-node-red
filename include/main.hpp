#pragma once
#include <iostream>
#include <sstream>
#include <iomanip>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_vfs_dev.h"

#include "cJSON.h"

#include "DHT.hpp"

#define LED_PIN GPIO_NUM_23
#define DHT_PIN GPIO_NUM_4
#define BTN_PIN GPIO_NUM_5

TaskHandle_t ISR = nullptr;
static bool led_status = false;

static SemaphoreHandle_t mutex;

void DHT_task(void *pvParameter);
void IRAM_ATTR button_isr_handler(void *pvParameter);
void button_task(void *pvParameter);
void serial_task(void *pvParameter);

extern "C"
{
    void app_main();
}