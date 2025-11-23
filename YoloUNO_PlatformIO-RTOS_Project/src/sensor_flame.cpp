#include "sensor_flame.h" 
#include "task_webserver.h"

void sensor_flame_task(void *pvParameters){
    Serial.println(">>> Task sensor_flame (Flame Sensor): Started");

    // Configure analog input pin for flame detection
    pinMode(FLAME_SENSOR_PIN, INPUT);
    
    while (1){
        // Read analog value for flame detection (0-4095 ADC)
        int rawValue = analogRead(FLAME_SENSOR_PIN);
    
        // Detect flame if signal is BELOW threshold (inverted logic)
        // Most flame sensors: LOW signal = flame detected, HIGH = safe
        const int FLAME_THRESHOLD = 2000;  // Adjust this value (0-4095)
        bool flameDetected = (rawValue < FLAME_THRESHOLD);  // Changed from > to <

        // ✅ Update global variable with mutex protection
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            glob_flame_detected = flameDetected;
            xSemaphoreGive(xMutex);
        }
        
        // ✅ Create JSON for WebSocket (send boolean)
        String jsonString = "";
        StaticJsonDocument<128> doc;
        doc["type"] = "sensor"; 
        doc["flame"] = flameDetected;  // Send true/false
        serializeJson(doc, jsonString);
        
        // Send to webserver
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString); 
        }

        // Serial output
        Serial.print("Flame - RAW: ");
        Serial.print(rawValue);
        Serial.print(" → Status: ");
        if (flameDetected) {
            Serial.println("⚠️ FIRE DETECTED!");
        } else {
            Serial.println("✓ Safe");
        }

        // Wait 500ms (faster update for safety)
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
