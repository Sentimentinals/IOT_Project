#ifndef __TEMP_HUMI_OLED_H__
#define __TEMP_HUMI_OLED_H__

#include "global.h" 
#include <Arduino.h>
#include <ArduinoJson.h>

// --- Driver Libraries ---
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- hardware config ---
#define DHTPIN 6   //port D4-D3
#define DHTTYPE DHT11

// oled lcd SSD1306
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 // in pixels  
#define OLED_RESET    -1 // Reset pin
#define SCREEN_ADDRESS 0x3C 


void temp_humi_oled(void *pvParameters);

#endif 