#include "neo_blinky.h"

// Hàm chọn màu LED dựa trên nhiệt độ
uint32_t getColorByTemperature(Adafruit_NeoPixel &strip, float temperature) {
    if (temperature >= 50.0) {
        return strip.Color(255, 0, 0);
    } 
    else if (temperature >= 40.0) {
        return strip.Color(255, 200, 0);
    } 
    else if (temperature >= 25.0) {
        return strip.Color(0, 255, 0);
    } 
    else if (temperature >= 15.0) {
        return strip.Color(0, 200, 255);
    } 
    else {
        return strip.Color(0, 0, 255);
    }
}

// Hàm chọn màu LED dựa trên độ ẩm (tùy chọn)
uint32_t getColorByHumidity(Adafruit_NeoPixel &strip, float humidity) {
    if (humidity >= 80.0) {
        return strip.Color(0, 0, 200);
    } 
    else if (humidity >= 60.0) {
        return strip.Color(0, 150, 255);
    } 
    else if (humidity >= 40.0) {
        return strip.Color(0, 255, 100);
    } 
    else if (humidity >= 20.0) {
        return strip.Color(255, 150, 0);
    } 
    else {
        return strip.Color(255, 0, 0);
    }
}

void neo_blinky(void *pvParameters){

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.show();
    strip.setBrightness(50);

    Serial.println("[NEO_BLINKY] NeoPixel initialized - Fire Alert Enabled");

    float currentTemp = 25.0;
    float currentHumidity = 50.0;
    uint32_t currentColor = strip.Color(0, 255, 0);

    while(1) {
        // Đọc nhiệt độ, độ ẩm và trạng thái lửa từ biến global
        bool ledEnabled = true;
        bool flameDetected = false;
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            currentTemp = glob_temperature;
            currentHumidity = glob_humidity;
            ledEnabled = glob_neoled_enabled;
            flameDetected = glob_flame_detected;
            xSemaphoreGive(xMutex);
        }

        // Kiểm tra nếu LED được bật
        if (ledEnabled) {
            if (flameDetected) {
                // 🔥 FIRE ALERT MODE: Bright solid RED
                currentColor = strip.Color(255, 0, 0);
                strip.setBrightness(255);  // Full brightness
            } else {
                // NORMAL MODE: Temperature-based color
                strip.setBrightness(50);
                currentColor = getColorByTemperature(strip, currentTemp);
            }
            
            strip.setPixelColor(0, currentColor);
            strip.show();
        } else {
            // TẮT LED
            strip.setPixelColor(0, 0);
            strip.show();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}