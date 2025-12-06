#ifndef __SENSOR_FLAME_H__
#define __SENSOR_FLAME_H__

#include "global.h" 
#include <Arduino.h>
#include <ArduinoJson.h>

// --- Hardware Config ---
#define FLAME_SENSOR_PIN 10   // D7-D8

void sensor_flame_task(void *pvParameters);

#endif
