#ifndef __SENSOR_FLAME_H__
#define __SENSOR_FLAME_H__

#include "global.h" 
#include <Arduino.h>
#include <ArduinoJson.h>

#define FLAME_SENSOR_PIN 10

void sensor_flame_task(void *pvParameters);

#endif
