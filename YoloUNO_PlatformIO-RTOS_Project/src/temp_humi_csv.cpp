#include "temp_humi_csv.h"
#include <time.h>

// Lấy timestamp (Unix epoch hoặc millis nếu chưa sync NTP)
static unsigned long getTimestamp()
{
    if (glob_ntp_synced)
    {
        time_t now = time(nullptr);
        if (now > 0) {
            return (unsigned long)now;  // Unix epoch (giây từ 1970)
        }
    }
    // Fallback: millis/1000 (tương đương giây từ lúc boot)
    return millis() / 1000;
}

void temp_humi_csv(void *pvParameters) {
    Serial.println(">>> Task temp_humi_csv: Started");
    
    // Tạo file CSV với header nếu chưa tồn tại
    if (!LittleFS.exists(CSV_FILE)) {
        File csvFile = LittleFS.open(CSV_FILE, "w");
        if (csvFile) {
            csvFile.println("timestamp,temperature,humidity");
            csvFile.close();
            Serial.println("✅ Đã tạo file CSV: " + String(CSV_FILE));
        } else {
            Serial.println("❌ Lỗi tạo file CSV!");
        }
    } else {
        Serial.println("📄 File CSV đã tồn tại: " + String(CSV_FILE));
    }
    
    while (1) {
        float temperature = 0;
        float humidity = 0;
        
        // Đọc dữ liệu từ biến global (có bảo vệ mutex)
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            temperature = glob_temperature;
            humidity = glob_humidity;
            xSemaphoreGive(xMutex);
        }
        
        // Chỉ ghi nếu dữ liệu hợp lệ (tránh ghi giá trị lỗi -1)
        if (temperature != -1.0 && humidity != -1.0) {
            // 📊 Ghi dữ liệu vào file CSV
            File csvFile = LittleFS.open(CSV_FILE, "a");  // Append mode
            if (csvFile) {
                // Format: timestamp (epoch), temperature, humidity
                csvFile.print(getTimestamp());  // Unix epoch hoặc giây từ boot
                csvFile.print(",");
                csvFile.print(temperature, 2);  // 2 chữ số thập phân
                csvFile.print(",");
                csvFile.println(humidity, 2);
                csvFile.close();
                
                Serial.printf("💾 CSV [%lu]: %.1f°C, %.1f%%\n", 
                    getTimestamp(), temperature, humidity);
            } else {
                Serial.println("❌ Lỗi ghi CSV!");
            }
        }
        
        // Chờ interval trước khi ghi tiếp
        vTaskDelay(pdMS_TO_TICKS(CSV_INTERVAL_MS));
    }
}

