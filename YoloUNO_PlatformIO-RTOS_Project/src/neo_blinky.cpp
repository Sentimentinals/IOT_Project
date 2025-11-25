#include "neo_blinky.h"

/**
 * NEOPIXEL LED - HUMIDITY COLOR MAPPING:
 * 
 * < 30%  : RED       - Too Dry (CRITICAL)
 * 30-40% : ORANGE    - Dry (WARNING)
 * 40-60% : GREEN     - Ideal (NORMAL)
 * 60-70% : CYAN      - Acceptable (NORMAL)
 * > 70%  : PURPLE    - Too Humid (WARNING - mold risk)
 * 
 * BEHAVIOR:
 * - FIRE: Solid BRIGHT RED (max brightness)
 * - CRITICAL/WARNING: Blinking with appropriate color
 * - NORMAL: Solid light (no blinking)
 */

uint32_t getColorByHumidity(Adafruit_NeoPixel &strip, float humidity) {
    if (humidity < 30.0) {
        return strip.Color(255, 0, 0);      // Red - Too Dry (Critical)
    } 
    else if (humidity < 40.0) {
        return strip.Color(255, 100, 0);    // Orange - Dry (Warning)
    } 
    else if (humidity <= 60.0) {
        return strip.Color(0, 255, 0);      // Green - Ideal (Normal)
    } 
    else if (humidity <= 70.0) {
        return strip.Color(0, 200, 255);    // Cyan - Acceptable (Normal)
    } 
    else {
        return strip.Color(150, 0, 255);    // Purple - Too Humid (Warning)
    }
}

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
    const unsigned long ALERT_BLINK_INTERVAL = 300;  // Fast blink for alerts

    while(1) {
        SystemState_t state = getSystemState();
        
        // Get latest sensor data from queue
        if (xSensorDataQueue != NULL) {
            xQueuePeek(xSensorDataQueue, &sensorData, pdMS_TO_TICKS(50));
        }

        // CHECK IF LED IS DISABLED - Keep it OFF
        if (!sensorData.neoled_enabled) {
            strip.setPixelColor(0, 0);
            strip.show();
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;  // Skip rest of loop, check again
        }
        
        // FIRE MODE - Solid BRIGHT RED (max brightness, no blinking)
        if (state == STATE_FIRE_ALERT || sensorData.flame_detected) {
            strip.setBrightness(255);  // Maximum brightness
            strip.setPixelColor(0, strip.Color(255, 0, 0));  // Solid bright red
            strip.show();
        }
        // WARNING/CRITICAL MODE - Blinking
        else if (state == STATE_WARNING || state == STATE_CRITICAL) {
            // Set brightness based on state
            strip.setBrightness(state == STATE_CRITICAL ? 100 : 70);
            
            // Blinking effect for alerts
            if (millis() - lastBlinkTime >= ALERT_BLINK_INTERVAL) {
                blinkState = !blinkState;
                lastBlinkTime = millis();
            }
            
            currentColor = getColorByHumidity(strip, sensorData.humidity);
            
            if (blinkState) {
                strip.setPixelColor(0, currentColor);
            } else {
                strip.setPixelColor(0, 0);  // OFF
            }
            strip.show();
        }
        // NORMAL MODE - Solid light (no blinking)
        else {
            strip.setBrightness(50);
            currentColor = getColorByHumidity(strip, sensorData.humidity);
            strip.setPixelColor(0, currentColor);  // Solid color
            strip.show();
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
