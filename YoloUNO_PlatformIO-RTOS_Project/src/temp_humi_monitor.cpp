#include "temp_humi_monitor.h"
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);
#include "task_webserver.h"


void temp_humi_monitor(void *pvParameters){
    Serial.println(">>> Task temp_humi_monitor: Started!");
    Wire.begin(11, 12);
    Serial.begin(115200);
    dht20.begin();
    
    while (1){
        /* code */
        
        dht20.read();
        // Reading temperature in Celsius
        float temperature = dht20.getTemperature();
        // Reading humidity
        float humidity = dht20.getHumidity();

        

        // Check if any reads failed and exit early
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity =  -1;
            //return;
        }
        String jsonString = "";
        //Update global variables for temperature and humidity
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            glob_temperature = temperature;
            glob_humidity = humidity;
            xSemaphoreGive(xMutex);

            StaticJsonDocument<128> doc;
            doc["type"] = "sensor"; 
            doc["temperature"] = glob_temperature;
            doc["humidity"] = glob_humidity;
            serializeJson(doc, jsonString);
        }
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString); // Gọi hàm từ task_webserver.cpp
        }
        // Print the results
        
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print("%  Temperature: ");
        Serial.print(temperature);
        Serial.println("°C");
        
        vTaskDelay(5000);
    }
    
}