#include "temp_humi_oled.h" 
#include "task_webserver.h"

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void temp_humi_oled(void *pvParameters){
    Serial.println(">>> Task temp_humi_monitor (DHT11 & OLED): Started");

    // Start I2C bus, GPIO 11 (SDA), GPIO 12 (SCL)
    Wire.begin(11, 12);
    dht.begin();
    
    // Start OLED Display
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed. Check connection."));
    } else {
        Serial.println(F("SSD1306 OLED display initialized."));
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println("Sensor Task Booted!");
        display.display();
        vTaskDelay(pdMS_TO_TICKS(1000)); // Show message for 1 sec
    }

    
    while (1){
        // read from DHT11
        float humidity = dht.readHumidity();
        float temperature = dht.readTemperature();

        // check if reads failed
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity = -1.0; // Set to error values
        }

        // update global var
        String jsonString = "";
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            glob_temperature = temperature;
            glob_humidity = humidity;
            
            StaticJsonDocument<128> doc;
            doc["type"] = "sensor"; 
            doc["temperature"] = glob_temperature;
            doc["humidity"] = glob_humidity;
            serializeJson(doc, jsonString);

            xSemaphoreGive(xMutex);
        }
        
        // send to webserver
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString); 
        }

        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print("%  Temperature: ");
        Serial.print(temperature);
        Serial.println("°C");

        // OLED display
        display.clearDisplay();
        display.setTextSize(2); // Taller text
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        
        display.print(F("Temp: "));
        display.print(temperature, 1); // 1 decimal place
        display.println(F(" C"));

        display.setCursor(0, 32); // Move to second half of screen
        display.print(F("Humi: "));
        display.print(humidity, 1); // 1 decimal place
        display.println(F(" %"));
        
        display.display(); // Push the buffer to the screen
        
        // Wait 5 seconds
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

