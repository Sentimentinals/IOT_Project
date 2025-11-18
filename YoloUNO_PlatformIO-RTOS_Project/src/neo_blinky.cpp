#include "neo_blinky.h"

// Hàm chọn màu LED dựa trên nhiệt độ
uint32_t getColorByTemperature(Adafruit_NeoPixel &strip, float temperature) {
    if (temperature >= 50.0) {
        // Trên 50°C: Đỏ (rất nóng)
        return strip.Color(255, 0, 0);
    } 
    else if (temperature >= 40.0) {
        // 40-50°C: Vàng (nóng)
        return strip.Color(255, 200, 0);
    } 
    else if (temperature >= 25.0) {
        // 25-40°C: Xanh lá (bình thường)
        return strip.Color(0, 255, 0);
    } 
    else if (temperature >= 15.0) {
        // 15-25°C: Xanh lam/Cyan (mát)
        return strip.Color(0, 200, 255);
    } 
    else {
        // Dưới 15°C: Xanh dương (lạnh)
        return strip.Color(0, 0, 255);
    }
}

// Hàm chọn màu LED dựa trên độ ẩm (tùy chọn)
uint32_t getColorByHumidity(Adafruit_NeoPixel &strip, float humidity) {
    if (humidity >= 80.0) {
        // Trên 80%: Xanh dương đậm (rất ẩm)
        return strip.Color(0, 0, 200);
    } 
    else if (humidity >= 60.0) {
        // 60-80%: Xanh lam (ẩm vừa)
        return strip.Color(0, 150, 255);
    } 
    else if (humidity >= 40.0) {
        // 40-60%: Xanh lá (bình thường)
        return strip.Color(0, 255, 100);
    } 
    else if (humidity >= 20.0) {
        // 20-40%: Vàng cam (khô)
        return strip.Color(255, 150, 0);
    } 
    else {
        // Dưới 20%: Đỏ (rất khô)
        return strip.Color(255, 0, 0);
    }
}

void neo_blinky(void *pvParameters){

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.show();
    strip.setBrightness(50); // Độ sáng cố định 50/255

    Serial.println("[NEO_BLINKY] NeoPixel initialized - Temperature Mode");

    float currentTemp = 25.0;    // Giá trị mặc định
    float currentHumidity = 50.0; // Giá trị mặc định
    uint32_t currentColor = strip.Color(0, 255, 0); // Xanh lá mặc định

    while(1) {
        // Đọc nhiệt độ và độ ẩm từ biến global (có bảo vệ mutex)
        bool ledEnabled = true;
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            currentTemp = glob_temperature;
            currentHumidity = glob_humidity;
            ledEnabled = glob_neoled_enabled;  // Đọc trạng thái LED
            xSemaphoreGive(xMutex);
        }

        // Kiểm tra nếu LED được bật
        if (ledEnabled) {
            // Chọn màu dựa trên nhiệt độ (ưu tiên nhiệt độ)
            currentColor = getColorByTemperature(strip, currentTemp);
            
            // Nếu muốn dùng độ ẩm thay vì nhiệt độ, uncomment dòng này:
            // currentColor = getColorByHumidity(strip, currentHumidity);

            strip.setPixelColor(0, currentColor);
            strip.show();
        } else {
            // TẮT LED
            strip.setPixelColor(0, 0);  // Màu đen (tắt)
            strip.show();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}