#include "neo_blinky.h"

/**
 * NEOPIXEL LED - HUMIDITY COLOR MAPPING (using centralized thresholds):
 * 
 * < HUMIDITY_CRITICAL_LOW (30%) : RED       - Too Dry (CRITICAL)
 * < HUMIDITY_WARNING_LOW (40%)  : ORANGE    - Dry (WARNING)
 * <= HUMIDITY_NORMAL_MAX (70%)  : GREEN     - Ideal (NORMAL)
 * <= HUMIDITY_WARNING_HIGH (80%): CYAN      - Acceptable (NORMAL)
 * > HUMIDITY_WARNING_HIGH       : PURPLE    - Too Humid (WARNING)
 * 
 * BEHAVIOR:
 * - FIRE: Solid BRIGHT RED (max brightness)
 * - CRITICAL/WARNING: Blinking with appropriate color
 * - NORMAL: Solid light (no blinking)
 */

uint32_t getColorByHumidity(Adafruit_NeoPixel &strip, float humidity) {
    if (humidity < HUMIDITY_CRITICAL_LOW) {
        return strip.Color(255, 0, 0);      // Red - Too Dry (Critical)
    } 
    else if (humidity < HUMIDITY_WARNING_LOW) {
        return strip.Color(255, 100, 0);    // Orange - Dry (Warning)
    } 
    else if (humidity <= HUMIDITY_NORMAL_MAX) {
        return strip.Color(0, 255, 0);      // Green - Ideal (Normal)
    } 
    else if (humidity <= HUMIDITY_WARNING_HIGH) {
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
