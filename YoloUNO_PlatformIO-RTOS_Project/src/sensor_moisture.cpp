#include "sensor_moisture.h" 
#include "task_webserver.h"

void sensor_moisture_task(void *pvParameters) {
    Serial.println("[Moisture] Task started");

    pinMode(MOISTURE_SENSOR_PIN, INPUT);
    SensorData_t sensorData = {0};
    
    while (1) {
        int rawValue = analogRead(MOISTURE_SENSOR_PIN);
        float moistureLevel = 100.0 - ((rawValue / 4095.0) * 100.0);

        if (xSensorDataQueue != NULL) {
            if (xQueuePeek(xSensorDataQueue, &sensorData, pdMS_TO_TICKS(50)) == pdTRUE) {
                sensorData.moisture_level = moistureLevel;
            }
        }
        
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
