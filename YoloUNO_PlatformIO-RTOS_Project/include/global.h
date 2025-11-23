#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ==================== SENSOR GLOBALS ====================
extern float glob_temperature;
extern float glob_humidity;
extern float glob_light_level;
extern float glob_moisture_level;
extern bool glob_flame_detected;
extern bool glob_fan_enabled;
extern bool glob_neoled_enabled;
extern bool glob_ntp_synced;

// ==================== WIFI & COREIOT CONFIG ====================
extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern int CORE_IOT_PORT;  // int, not String

// ==================== LEGACY VARIABLES ====================
extern String ssid;
extern String password;
extern String wifi_ssid;
extern String wifi_password;
extern boolean isWifiConnected;

// ==================== RTOS SEMAPHORES ====================
extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern SemaphoreHandle_t xMutex;
extern SemaphoreHandle_t xI2CMutex;

#endif