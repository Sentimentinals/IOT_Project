#ifndef __SENSOR_WATER_PUMP_H__
#define __SENSOR_WATER_PUMP_H__

#include "global.h" 
#include <Arduino.h>
#include <ArduinoJson.h>

// --- Hardware Config ---
#define WATER_PUMP_PIN GPIO_NUM_38   // Water pump relay pin

// --- Soil Moisture Thresholds ---
#define SOIL_DRY_THRESHOLD 30.0      // Below this = dry soil, auto pump ON
#define SOIL_WET_THRESHOLD 60.0      // Above this = wet enough, auto pump OFF

// Task function
void sensor_water_pump_task(void *pvParameters);

// Manual control function (call from WebSocket handler)
#ifdef __cplusplus
extern "C" {
#endif
void setWaterPumpManual(bool enabled);
#ifdef __cplusplus
}
#endif

#endif

