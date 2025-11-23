#include "sensor_moisture.h" 
#include "task_webserver.h"

void sensor_moisture_task(void *pvParameters){
    Serial.println(">>> Task sensor_moisture (Soil Moisture): Started");

    // Configure ADC pin
    pinMode(MOISTURE_SENSOR_PIN, INPUT);
    
    while (1){
        // Read analog value (0-4095 for ESP32 12-bit ADC)
        int rawValue = analogRead(MOISTURE_SENSOR_PIN);
        
        // Convert to moisture percentage (0-100%)
        // For most capacitive sensors: lower ADC value = more moisture
        // Invert the reading: 0 ADC = 100% wet, 4095 ADC = 0% dry
        float moistureLevel = 100.0 - ((rawValue / 4095.0) * 100.0);

        // ✅ Update global variable with mutex protection
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            glob_moisture_level = moistureLevel;
            xSemaphoreGive(xMutex);
        }
        
        // ✅ Create JSON for WebSocket (outside mutex)
        String jsonString = "";
        StaticJsonDocument<128> doc;
        doc["type"] = "sensor"; 
        doc["moisture"] = moistureLevel;
        serializeJson(doc, jsonString);
        
        // Send to webserver
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString); 
        }

        Serial.print("Soil Moisture: ");
        Serial.print(moistureLevel, 1);
        Serial.println("%");

        // Wait 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
