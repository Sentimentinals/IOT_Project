#include "neo_blinky.h"

uint32_t getColorByHumidity(Adafruit_NeoPixel &strip, float humidity) {
    if (humidity < HUMIDITY_CRITICAL_LOW) {
        return strip.Color(255, 0, 0);
    } 
    else if (humidity < HUMIDITY_WARNING_LOW) {
        return strip.Color(255, 100, 0);
    } 
    else if (humidity <= HUMIDITY_NORMAL_MAX) {
        return strip.Color(0, 255, 0);
    } 
    else if (humidity <= HUMIDITY_WARNING_HIGH) {
        return strip.Color(0, 200, 255);
    } 
    else {
        return strip.Color(150, 0, 255);
    }
}

// FreeRTOS task: Control NeoPixel color based on humidity level
void neo_blinky(void *pvParameters) {
    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.show();
    strip.setBrightness(50);

    Serial.println("[NeoPixel] Started");

    SensorData_t sensorData = {0};
    sensorData.neoled_enabled = true;
    
    uint32_t currentColor = strip.Color(0, 255, 0);
    bool blinkState = false;
    unsigned long lastBlinkTime = 0;
    const unsigned long ALERT_BLINK_INTERVAL = 300;

    while(1) {
        SystemState_t state = getSystemState();
        
        if (xSensorDataQueue != NULL) {
            xQueuePeek(xSensorDataQueue, &sensorData, pdMS_TO_TICKS(50));
        }

        if (!sensorData.neoled_enabled) {
            strip.setPixelColor(0, 0);
            strip.show();
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        if (state == STATE_FIRE_ALERT || sensorData.flame_detected) {
            strip.setBrightness(255);
            strip.setPixelColor(0, strip.Color(255, 0, 0));
            strip.show();
        }
        else if (state == STATE_WARNING || state == STATE_CRITICAL) {
            strip.setBrightness(state == STATE_CRITICAL ? 100 : 70);
            
            if (millis() - lastBlinkTime >= ALERT_BLINK_INTERVAL) {
                blinkState = !blinkState;
                lastBlinkTime = millis();
            }
            
            currentColor = getColorByHumidity(strip, sensorData.humidity);
            
            if (blinkState) {
                strip.setPixelColor(0, currentColor);
            } else {
                strip.setPixelColor(0, 0);
            }
            strip.show();
        }
        else {
            strip.setBrightness(50);
            currentColor = getColorByHumidity(strip, sensorData.humidity);
            strip.setPixelColor(0, currentColor);
            strip.show();
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
