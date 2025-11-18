#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern float glob_temperature;
extern float glob_humidity;

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

extern SemaphoreHandle_t xMutex;
extern SemaphoreHandle_t xI2CMutex; // Semaphore cho I2C bus

// Điều khiển NeoPixel LED
extern bool glob_neoled_enabled;
extern bool glob_ntp_synced; // Đánh dấu đã đồng bộ NTP

#endif