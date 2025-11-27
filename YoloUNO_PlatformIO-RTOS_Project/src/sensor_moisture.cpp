#include "sensor_moisture.h" 
#include "task_webserver.h"

void sensor_moisture_task(void *pvParameters) {
    Serial.println("[Moisture] Task started");

    pinMode(MOISTURE_SENSOR_PIN, INPUT);
    
    while (1) {
        int rawValue = analogRead(MOISTURE_SENSOR_PIN);
        float moistureLevel = 100.0 - ((rawValue / 4095.0) * 100.0);

        // Thread-safe update of moisture_level field only
        updateSensorField_Moisture(moistureLevel);
        
        // Send to WebSocket
        String jsonString = "";
        StaticJsonDocument<128> doc;
        doc["type"] = "sensor"; 
        doc["moisture"] = moistureLevel;
        serializeJson(doc, jsonString);
        
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString); 
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
