#include "sensor_light.h" 
#include "task_webserver.h"

void sensor_light_task(void *pvParameters){
    Serial.println(">>> Task sensor_light (Photoresistor): Started");

    // Configure ADC pin
    pinMode(LIGHT_SENSOR_PIN, INPUT);
    
    while (1){
        // Read analog value (0-4095 for ESP32 12-bit ADC)
        int rawValue = analogRead(LIGHT_SENSOR_PIN);
        float lightLevel = (rawValue / 4095.0) * 100.0;

        // ✅ Update global variable with mutex protection
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            glob_light_level = lightLevel;
            xSemaphoreGive(xMutex);
        }

        // Estimate Lux value (approximate conversion for photoresistor)
        // Typical photoresistor: 0-4095 ADC ≈ 0-1000 Lux (very rough estimate)
        float luxEstimate = (rawValue / 4095.0) * 1000.0;

        // Send to webserver
        String jsonString = "";
        StaticJsonDocument<128> doc;
        doc["type"] = "sensor"; 
        doc["light"] = luxEstimate;  // Send Lux instead of %
        serializeJson(doc, jsonString);
        
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString); 
        }

        Serial.print("Light - RAW: ");
        Serial.print(rawValue);
        Serial.print(" ADC | Level: ");
        Serial.print(lightLevel, 1);
        Serial.print("% | ~");
        Serial.print(luxEstimate, 0);
        Serial.println(" Lux");

        // Wait 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
