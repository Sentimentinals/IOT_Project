#include "sensor_flame.h" 
#include "task_webserver.h"

/**
 * FLAME SENSOR TASK
 * Fire Alert has HIGHEST priority - overrides all other states
 */

void sensor_flame_task(void *pvParameters) {
    Serial.println("[Flame] Task started");

    pinMode(FLAME_SENSOR_PIN, INPUT);
    
    bool lastFlameState = false;
    SensorData_t sensorData = {0};
    
    while (1) {
        int rawValue = analogRead(FLAME_SENSOR_PIN);
        const int FLAME_THRESHOLD = 2000;
        bool flameDetected = (rawValue < FLAME_THRESHOLD);

        sensorData.flame_detected = flameDetected;
        
        // State change detection
        if (flameDetected != lastFlameState) {
            lastFlameState = flameDetected;
            
            if (flameDetected) {
                Serial.println("[Flame] FIRE DETECTED!");
                updateSystemState(STATE_FIRE_ALERT);
            } else {
                Serial.println("[Flame] Fire cleared");
            }
        }
        
        // Update queue
        if (xSensorDataQueue != NULL) {
            SensorData_t currentData;
            if (xQueuePeek(xSensorDataQueue, &currentData, 0) == pdTRUE) {
                currentData.flame_detected = flameDetected;
                xQueueOverwrite(xSensorDataQueue, &currentData);
            }
        }
        
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
