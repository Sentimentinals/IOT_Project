#include "temp_humi_oled.h" 
#include "task_webserver.h"

DHT20 dht20;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static unsigned long lastAnimTime = 0;
static bool animState = false;
static int animFrame = 0;
static SystemState_t lastSentState = STATE_NORMAL;

// Get status text for normal conditions
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

// Get warning message based on sensor readings
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

void sendStateAlert(SystemState_t state, float temp, float humidity) {
    String jsonString = "";
    StaticJsonDocument<256> doc;
    doc["type"] = "state_alert";
    doc["state"] = (int)state;
    doc["temperature"] = temp;
    doc["humidity"] = humidity;
    
    switch (state) {
        case STATE_WARNING:
            doc["level"] = "warning";
            doc["reason"] = getWarningReason(temp, humidity);
            break;
        case STATE_CRITICAL:
            doc["level"] = "critical";
            if (temp > 35.0) doc["reason"] = "Too Hot";
            else if (humidity < 30.0) doc["reason"] = "Too Dry";
            else doc["reason"] = "Critical";
            break;
        case STATE_FIRE_ALERT:
            doc["level"] = "fire";
            doc["reason"] = "Fire detected";
            break;
        default:
            doc["level"] = "normal";
            doc["reason"] = "All clear";
            break;
    }
    
    serializeJson(doc, jsonString);
    Webserver_sendata(jsonString);
}

// FreeRTOS task: Display sensor data on OLED and evaluate system state
void temp_humi_oled(void *pvParameters) {
    Serial.println("[OLED-DHT20] Task started");

    if (xI2CMutex != NULL && xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        Wire.begin();
        dht20.begin();
        
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
        
        if (dht20.isConnected()) {
            Serial.println("[DHT20] Connected at 0x38");
        } else {
            Serial.println("[DHT20] Not connected!");
        }
        xSemaphoreGive(xI2CMutex);
    } else {
        Serial.println("[OLED] Failed to get I2C mutex");
    }
    
    vTaskDelay(pdMS_TO_TICKS(1500));

    SensorData_t sensorData = {0};
    bool sensorReady = false;
    
    while (1) {
        float humidity = 0;
        float temperature = 0;
        
        if (xI2CMutex != NULL && xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            int status = dht20.read();
            if (status == DHT20_OK) {
                humidity = dht20.getHumidity();
                temperature = dht20.getTemperature();
            }
            xSemaphoreGive(xI2CMutex);
        }

        bool validReading = !isnan(temperature) && !isnan(humidity) && 
                           temperature > 0 && temperature < 80 &&
                           humidity > 0 && humidity <= 100;

        if (!validReading) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        
        if (!sensorReady) {
            sensorReady = true;
            lastSentState = STATE_NORMAL;
            Serial.println("[OLED] Sensor ready");
        }

        sensorData.temperature = temperature;
        sensorData.humidity = humidity;
        
        if (xSensorDataQueue != NULL) {
            SensorData_t queueData;
            if (xQueuePeek(xSensorDataQueue, &queueData, pdMS_TO_TICKS(50)) == pdTRUE) {
                sensorData.flame_detected = queueData.flame_detected;
            }
        }
        
        SystemState_t newState = evaluateSystemState(temperature, humidity, sensorData.flame_detected);
        updateSystemState(newState);
        
        if (sensorReady && newState != lastSentState) {
            if (newState >= STATE_WARNING) {
                sendStateAlert(newState, temperature, humidity);
            } else if (lastSentState >= STATE_WARNING && newState == STATE_NORMAL) {
                sendStateAlert(STATE_NORMAL, temperature, humidity);
            }
            lastSentState = newState;
        }
        
        sendSensorData(&sensorData);
        
        String jsonString = "";
        StaticJsonDocument<128> doc;
        doc["type"] = "sensor"; 
        doc["temperature"] = temperature;
        doc["humidity"] = humidity;
        serializeJson(doc, jsonString);
        
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString); 
        }

        if (millis() - lastAnimTime >= 500) {
            animState = !animState;
            animFrame++;
            lastAnimTime = millis();
        }

        if (xI2CMutex != NULL && xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
            SystemState_t state = getSystemState();
            
            switch (state) {
                case STATE_FIRE_ALERT:
                    drawFireAlertDisplay(animFrame);
                    break;
                case STATE_CRITICAL:
                    drawCriticalDisplay(temperature, humidity, animFrame);
                    break;
                case STATE_WARNING:
                    drawWarningDisplay(temperature, humidity, animState);
                    break;
                case STATE_NORMAL:
                default:
                    drawNormalDisplay(temperature, humidity);
                    break;
            }
            
            xSemaphoreGive(xI2CMutex);
        }
        
        if (getSystemState() >= STATE_WARNING) {
            vTaskDelay(pdMS_TO_TICKS(300));
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}