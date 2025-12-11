#ifndef __TEMP_HUMI_MONITOR_H__
#define __TEMP_HUMI_MONITOR_H__

#include "global.h"
#include <Wire.h>
#include "DHT20.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

void temp_humi_monitor(void *pvParameters);

#endif

