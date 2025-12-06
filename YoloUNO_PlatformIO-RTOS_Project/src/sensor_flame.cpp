#include "sensor_flame.h" 
#include "task_webserver.h"

#define FLAME_DEBOUNCE_COUNT 3
#define FLAME_THRESHOLD 1500

void sensor_flame_task(void *pvParameters) {
    Serial.println("[Flame] Task started");

    pinMode(FLAME_SENSOR_PIN, INPUT);
    
    bool lastFlameState = false;
    bool confirmedFlameState = false;
    int flameCounter = 0;
    int noFlameCounter = 0;
    
    int initValue = analogRead(FLAME_SENSOR_PIN);
    Serial.printf("[Flame] Initial: %d (threshold: %d)\n", initValue, FLAME_THRESHOLD);
    
    while (1) {
        int rawValue = analogRead(FLAME_SENSOR_PIN);
        bool currentReading = (rawValue < FLAME_THRESHOLD);
        
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
        
        if (flameDetected != lastFlameState) {
            lastFlameState = flameDetected;
            if (flameDetected) {
                updateSystemState(STATE_FIRE_ALERT);
            }
        }
        
        updateSensorField_Flame(flameDetected);
        
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
