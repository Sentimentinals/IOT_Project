#include "temp_humi_monitor.h"
#include "task_webserver.h"

DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);

void temp_humi_monitor(void *pvParameters){
    Serial.println("[DHT20] Task started");
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Initialize DHT20 with I2C mutex
    if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        dht20.begin();
        xSemaphoreGive(xI2CMutex);
    }
    
    SensorData_t sensorData = {0};
    
    while (1){
        float temperature = -1;
        float humidity = -1;
        
        // Read DHT20 with I2C mutex
        if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            dht20.read();
            temperature = dht20.getTemperature();
            humidity = dht20.getHumidity();
            xSemaphoreGive(xI2CMutex);
        }

        if (isnan(temperature) || isnan(humidity)) {
            temperature = humidity = -1;
        }
        
        // Update sensor data structure
        sensorData.temperature = temperature;
        sensorData.humidity = humidity;
        
        // Send to queue for other tasks
        sendSensorData(&sensorData);
        
        // Send to WebSocket
        String jsonString = "";
        StaticJsonDocument<128> doc;
        doc["type"] = "sensor";
        doc["temperature"] = temperature;
        doc["humidity"] = humidity;
        serializeJson(doc, jsonString);
        
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
