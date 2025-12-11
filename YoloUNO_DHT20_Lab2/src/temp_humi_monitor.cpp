#include "temp_humi_monitor.h"
#include "task_webserver.h"

#define LED_PIN 48

DHT20 dht20;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static unsigned long lastAnimTime = 0;
static bool animState = false;
static int animFrame = 0;

int getBlinkDelay(float temperature) {
    if (temperature < 30) return 1000;      // Cold: Slow
    else if (temperature <= 33) return 500; // Normal
    else if (temperature <= 36) return 200; // Warning: Fast
    else return 100;                        // Critical: Very fast
}

const char* getNormalStatus(float temp, float humidity) {
    if (temp >= 25.0 && temp <= TEMP_NORMAL_MAX && 
        humidity >= HUMIDITY_NORMAL_MIN && humidity <= HUMIDITY_NORMAL_MAX) {
        return "IDEAL";
    }
    if (temp >= TEMP_NORMAL_MIN && temp <= TEMP_NORMAL_MAX && 
        humidity >= HUMIDITY_NORMAL_MIN && humidity <= HUMIDITY_WARNING_HIGH) {
        return "GOOD";
    }
    return "OK";
}

const char* getDisplayWarning(float temp, float humidity) {
    if (temp > TEMP_CRITICAL) return "TOO HOT!";
    if (temp > TEMP_HOT) return "HOT";
    if (temp < TEMP_COLD) return "COLD";
    if (temp < TEMP_NORMAL_MIN) return "COOL";
    if (humidity < HUMIDITY_CRITICAL_LOW) return "TOO DRY!";
    if (humidity < HUMIDITY_WARNING_LOW) return "DRY";
    if (humidity > HUMIDITY_WARNING_HIGH) return "TOO HUMID";
    return "WARNING";
}

void drawNormalDisplay(float temp, float humidity) {
    display.clearDisplay();
    
    const char* status = getNormalStatus(temp, humidity);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(status);
    
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    
    display.setTextSize(2);
    display.setCursor(0, 16);
    display.print(temp, 1);
    display.setTextSize(1);
    display.print(F(" C"));
    
    display.setTextSize(2);
    display.setCursor(0, 40);
    display.print(humidity, 1);
    display.setTextSize(1);
    display.print(F(" %"));
    
    display.setCursor(100, 28);
    display.setTextSize(2);
    if (temp >= 25.0 && temp <= 30.0 && humidity >= 40.0 && humidity <= HUMIDITY_NORMAL_MAX) {
        display.print(F(":D"));
    } else {
        display.print(F(":)"));
    }
    
    display.display();
}

void drawWarningDisplay(float temp, float humidity, bool borderOn) {
    display.clearDisplay();
    
    if (borderOn) {
        display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
        display.drawRect(1, 1, 126, 62, SSD1306_WHITE);
    }
    
    const char* warning = getDisplayWarning(temp, humidity);
    display.setTextSize(1);
    display.setCursor(10, 4);
    display.print(F("!! "));
    display.print(warning);
    display.print(F(" !!"));
    
    display.setTextSize(2);
    display.setCursor(8, 18);
    display.print(F("T:"));
    display.print(temp, 1);
    display.setTextSize(1);
    display.print(F("C"));
    
    display.setTextSize(2);
    display.setCursor(8, 40);
    display.print(F("H:"));
    display.print(humidity, 1);
    display.setTextSize(1);
    display.print(F("%"));
    
    display.setTextSize(2);
    display.setCursor(105, 25);
    display.print(F("!"));
    
    display.display();
}

void drawCriticalDisplay(float temp, float humidity, int frame) {
    display.clearDisplay();
    
    if (frame % 2 == 0) {
        display.fillRect(0, 0, 128, 16, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
    } else {
        display.setTextColor(SSD1306_WHITE);
    }
    
    display.setTextSize(2);
    display.setCursor(8, 0);
    display.print(F("CRITICAL"));
    
    display.setTextColor(SSD1306_WHITE);
    
    display.setTextSize(2);
    display.setCursor(0, 24);
    display.print(temp, 1);
    display.print(F("C "));
    display.print(humidity, 0);
    display.print(F("%"));
    
    display.setTextSize(1);
    display.setCursor(0, 48);
    if (temp > TEMP_CRITICAL) {
        display.print(F("TOO HOT! Cool down!"));
    } else if (humidity < HUMIDITY_CRITICAL_LOW) {
        display.print(F("TOO DRY! Add moisture!"));
    } else {
        display.print(F("Check environment!"));
    }
    
    if (frame % 2 == 0) {
        display.setTextSize(2);
        display.setCursor(112, 24);
        display.print(F("!"));
    }
    
    display.display();
}

void drawFireAlertDisplay(int frame) {
    display.clearDisplay();
    
    display.fillTriangle(20, 50, 35, 20, 50, 50, SSD1306_WHITE);
    display.fillTriangle(25, 50, 35, 30, 45, 50, SSD1306_BLACK);
    display.fillTriangle(30, 50, 35, 35, 40, 50, SSD1306_WHITE);
    
    display.setTextSize(2);
    display.setCursor(60, 8);
    display.print(F("FIRE"));
    
    display.setTextSize(1);
    display.setCursor(60, 28);
    display.print(F("DETECTED!"));
    
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("!! EVACUATE NOW !!"));
    
    if (frame % 2 == 0) {
        display.setTextSize(2);
        display.setCursor(112, 8);
        display.print(F("!"));
    }
    
    display.display();
}

void temp_humi_monitor(void *pvParameters){
    Serial.println("[DHT20] Task started");
    
    pinMode(LED_PIN, OUTPUT);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Initialize DHT20 với I2C mutex
    if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        dht20.begin();
        xSemaphoreGive(xI2CMutex);
    }
    
    // Initialize OLED với I2C mutex
    if (xI2CMutex != NULL && xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
            Serial.println("[OLED] Init failed");
        } else {
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);
            display.println(F("Initializing..."));
            display.println(F(""));
            display.println(F("Temp/Humidity Monitor"));
            display.println(F("DHT20 + SSD1306"));
            display.display();
            Serial.println("[OLED] Init OK");
        }
        xSemaphoreGive(xI2CMutex);
    } else {
        Serial.println("[OLED] Failed to get I2C mutex");
    }
    
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    SensorData_t sensorData = {0};
    bool ledState = false;
    int blinkDelay = 500;
    unsigned long lastBlink = 0;
    unsigned long lastRead = 0;
    SystemState_t lastState = STATE_NORMAL;
    
    while (1){
        unsigned long now = millis();
        
        // Đọc DHT20 mỗi 5 giây
        if (now - lastRead >= 5000) {
            lastRead = now;
            float temperature = -1;
            float humidity = -1;
            
            if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                dht20.read();
                temperature = dht20.getTemperature();
                humidity = dht20.getHumidity();
                xSemaphoreGive(xI2CMutex);
            }

            if (isnan(temperature) || isnan(humidity)) {
                temperature = humidity = -1;
            }
            
            sensorData.temperature = temperature;
            sensorData.humidity = humidity;
            sendSensorData(&sensorData);
            
            // Đánh giá system state
            SystemState_t newState = evaluateSystemState(temperature, humidity, false);
            updateSystemState(newState);
            lastState = newState;
            
            // Cập nhật blink delay theo nhiệt độ
            if (temperature > -1) {
                blinkDelay = getBlinkDelay(temperature);
                Serial.printf("[DHT20] Temp: %.1fC, Humi: %.1f%%, Blink: %dms\n", 
                              temperature, humidity, blinkDelay);
            }
            
            String jsonString = "";
            StaticJsonDocument<128> doc;
            doc["type"] = "sensor";
            doc["temperature"] = temperature;
            doc["humidity"] = humidity;
            serializeJson(doc, jsonString);
            
            if (jsonString.length() > 0) {
                Webserver_sendata(jsonString);
            }
        }
        
        // Animation update
        if (millis() - lastAnimTime >= 500) {
            animState = !animState;
            animFrame++;
            lastAnimTime = millis();
        }
        
        // Hiển thị OLED với I2C mutex
        if (xI2CMutex != NULL && xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            SystemState_t state = getSystemState();
            float temp = sensorData.temperature;
            float humi = sensorData.humidity;
            
            if (temp > -1 && humi > -1) {
                switch (state) {
                    case STATE_FIRE_ALERT:
                        drawFireAlertDisplay(animFrame);
                        break;
                    case STATE_CRITICAL:
                        drawCriticalDisplay(temp, humi, animFrame);
                        break;
                    case STATE_WARNING:
                        drawWarningDisplay(temp, humi, animState);
                        break;
                    case STATE_NORMAL:
                    default:
                        drawNormalDisplay(temp, humi);
                        break;
                }
            }
            
            xSemaphoreGive(xI2CMutex);
        }
        
        // LED blink theo nhiệt độ
        if (now - lastBlink >= blinkDelay) {
            lastBlink = now;
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

