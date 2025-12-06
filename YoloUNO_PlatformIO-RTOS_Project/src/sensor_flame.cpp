#include "sensor_flame.h" 
#include "task_webserver.h"

/**
 * FLAME SENSOR TASK
 * Fire Alert has HIGHEST priority - overrides all other states
 * 
 * Flame sensor (KY-026 / similar):
 * - Normal (no flame): HIGH voltage (~3.3V) → rawValue HIGH (3000-4095)
 * - Flame detected: LOW voltage → rawValue LOW (< threshold)
 * 
 * Added debounce: Requires 3 consecutive readings to confirm state change
 */

// Debounce configuration
#define FLAME_DEBOUNCE_COUNT 3
#define FLAME_THRESHOLD 1500

void sensor_flame_task(void *pvParameters) {
    Serial.println("[Flame] Task started");

    pinMode(FLAME_SENSOR_PIN, INPUT);
    
    bool lastFlameState = false;
    bool confirmedFlameState = false;
    int flameCounter = 0;
    int noFlameCounter = 0;
    
    // Initial reading
    int initValue = analogRead(FLAME_SENSOR_PIN);
    Serial.printf("[Flame] Initial: %d (threshold: %d)\n", initValue, FLAME_THRESHOLD);
    
    while (1) {
        int rawValue = analogRead(FLAME_SENSOR_PIN);
        bool currentReading = (rawValue < FLAME_THRESHOLD);
        
        // Debounce logic
        if (currentReading) {
            flameCounter++;
            noFlameCounter = 0;
            if (flameCounter >= FLAME_DEBOUNCE_COUNT && !confirmedFlameState) {
                confirmedFlameState = true;
                Serial.printf("[Flame] FIRE DETECTED! (raw: %d)\n", rawValue);
            }
        } else {
            noFlameCounter++;
            flameCounter = 0;
            if (noFlameCounter >= FLAME_DEBOUNCE_COUNT && confirmedFlameState) {
                confirmedFlameState = false;
                Serial.printf("[Flame] Fire cleared (raw: %d)\n", rawValue);
            }
        }
        
        bool flameDetected = confirmedFlameState;
        
        // State change detection
        if (flameDetected != lastFlameState) {
            lastFlameState = flameDetected;
            if (flameDetected) {
                updateSystemState(STATE_FIRE_ALERT);
            }
        }
        
        // Thread-safe update of flame_detected field only
        updateSensorField_Flame(flameDetected);
        
        // Send to WebSocket
        String jsonString = "";
        StaticJsonDocument<128> doc;
        doc["type"] = "sensor"; 
        doc["flame"] = flameDetected;
        serializeJson(doc, jsonString);
        
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString); 
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
