#include "temp_humi_monitor.h"
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);
#include "task_webserver.h"


void temp_humi_monitor(void *pvParameters){
    Serial.println(">>> Task temp_humi_monitor: Started!");
    
    // Đợi I2C bus đã được khởi tạo
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Khởi tạo DHT20 (cần bảo vệ bằng semaphore vì dùng I2C)
    if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        dht20.begin();
        xSemaphoreGive(xI2CMutex);
        Serial.println("DHT20 initialized");
    }
    
    while (1){
        /* code */
        float temperature = -1;
        float humidity = -1;
        
        // Đọc cảm biến DHT20 (cần bảo vệ vì dùng I2C)
        if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            dht20.read();
            temperature = dht20.getTemperature();
            humidity = dht20.getHumidity();
            xSemaphoreGive(xI2CMutex);
        }

        

        // Check if any reads failed and exit early
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity =  -1;
            //return;
        }
        // ✅ TỐI ƯU: Chỉ giữ mutex khi ghi biến global
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            glob_temperature = temperature;
            glob_humidity = humidity;
            xSemaphoreGive(xMutex);  // Unlock ngay sau khi ghi xong
        }
        
        // ✅ TỐI ƯU: Tạo JSON bên ngoài mutex (dùng biến local)
        String jsonString = "";
        StaticJsonDocument<128> doc;
        doc["type"] = "sensor";
        doc["temperature"] = temperature;  // Dùng local variable
        doc["humidity"] = humidity;
        serializeJson(doc, jsonString);
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString); // Gọi hàm từ task_webserver.cpp
        }
        // Print the results
        Serial.print("[DHT20] Humidity: ");
        Serial.print(humidity);
        Serial.print("%  Temperature: ");
        Serial.print(temperature);
        Serial.println("°C");
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    
}