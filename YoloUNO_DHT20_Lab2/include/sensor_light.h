#ifndef __SENSOR_LIGHT_H__
#define __SENSOR_LIGHT_H__

#include "global.h" 
#include <Arduino.h>
#include <ArduinoJson.h>

#define LIGHT_SENSOR_PIN GPIO_NUM_1

void sensor_light_task(void *pvParameters);

#endif
