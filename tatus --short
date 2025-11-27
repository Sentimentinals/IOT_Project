#include "sensor_light.h" 
#include "task_webserver.h"

void sensor_light_task(void *pvParameters) {
    Serial.println("[Light] Task started");

    pinMode(LIGHT_SENSOR_PIN, INPUT);
    SensorData_t sensorData = {0};
    
    while (1) {
        int rawValue = analogRead(LIGHT_SENSOR_PIN);
        float lightLevel = (rawValue / 4095.0) * 100.0;
        float luxEstimate = (rawValue / 4095.0) * 1000.0;

        if (xSensorDataQueue != NULL) {
            if (xQueuePeek(xSensorDataQueue, &sensorData, pdMS_TO_TICKS(50)) == pdTRUE) {
                sensorData.light_level = luxEstimate;
            }
        }

        String jsonString = "";
        StaticJsonDocument<128> doc;
        doc["type"] = "sensor"; 
        doc["light"] = luxEstimate;
        serializeJson(doc, jsonString);
        
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString); 
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
