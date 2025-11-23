#ifndef __SENSOR_MOISTURE_H__
#define __SENSOR_MOISTURE_H__

#include "global.h" 
#include <Arduino.h>
#include <ArduinoJson.h>

// --- Hardware Config ---
#define MOISTURE_SENSOR_PIN GPIO_NUM_8   // D5-D6

void sensor_moisture_task(void *pvParameters);

#endif
