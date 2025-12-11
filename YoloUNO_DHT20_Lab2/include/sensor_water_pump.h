#ifndef __SENSOR_WATER_PUMP_H__
#define __SENSOR_WATER_PUMP_H__

#include "global.h" 
#include <Arduino.h>
#include <ArduinoJson.h>

#define WATER_PUMP_PIN GPIO_NUM_38

#define SOIL_DRY_THRESHOLD 30.0
#define SOIL_WET_THRESHOLD 60.0

void sensor_water_pump_task(void *pvParameters);

#ifdef __cplusplus
extern "C" {
#endif
void setWaterPumpManual(bool enabled);
#ifdef __cplusplus
}
#endif

#endif

