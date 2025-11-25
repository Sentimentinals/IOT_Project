#include <task_handler.h>

void handleWebSocketMessage(String message)
{
    Serial.println(message);
    StaticJsonDocument<256> doc;

    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        Serial.println("❌ Lỗi parse JSON!");
        return;
    }
    JsonObject value = doc["value"];
    if (doc["page"] == "device")
    {
        if (!value.containsKey("gpio") || !value.containsKey("status"))
        {
            Serial.println("⚠️ JSON thiếu thông tin gpio hoặc status");
            return;
        }

        int gpio = value["gpio"];
        String status = value["status"].as<String>();

        Serial.printf("⚙️ Điều khiển GPIO %d → %s\n", gpio, status.c_str());
        pinMode(gpio, OUTPUT);
        if (status.equalsIgnoreCase("ON"))
        {
            digitalWrite(gpio, HIGH);
            Serial.printf("🔆 GPIO %d ON\n", gpio);
        }
        else if (status.equalsIgnoreCase("OFF"))
        {
            digitalWrite(gpio, LOW);
            Serial.printf("💤 GPIO %d OFF\n", gpio);
        }
    }
    else if (doc["page"] == "wifi_setting")
    {
        String WIFI_SSID = doc["value"]["ssid"].as<String>();
        String WIFI_PASS = doc["value"]["password"].as<String>();

        Serial.println("📥 Received WiFi config:");
        Serial.println("SSID: " + WIFI_SSID);
        Serial.println("PASS: " + WIFI_PASS);

        Save_wifi_File(WIFI_SSID, WIFI_PASS);

        String msg = "{\"status\":\"ok\",\"page\":\"wifi_saved\"}";
        ws.textAll(msg);
    }
    else if (doc["page"] == "coreiot_setting")
    {
        String CORE_IOT_TOKEN = doc["value"]["token"].as<String>();
        String CORE_IOT_SERVER = doc["value"]["server"].as<String>();
        String CORE_IOT_PORT = doc["value"]["port"].as<String>();

        Serial.println("📥 Received CoreIOT config:");
        Serial.println("TOKEN: " + CORE_IOT_TOKEN);
        Serial.println("SERVER: " + CORE_IOT_SERVER);
        Serial.println("PORT: " + CORE_IOT_PORT);

        Save_coreiot_File(CORE_IOT_TOKEN, CORE_IOT_SERVER, CORE_IOT_PORT);

        String msg = "{\"status\":\"ok\",\"page\":\"coreiot_saved\"}";
        ws.textAll(msg);
    }
    else if (doc["page"] == "neoled")
    {
        // Điều khiển NeoPixel LED
        bool enabled = doc["value"]["enabled"].as<bool>();
        
        Serial.printf("💡 NeoLED Control: %s\n", enabled ? "BẬT" : "TẮT");
        
        // Cập nhật biến global (có bảo vệ mutex)
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            glob_neoled_enabled = enabled;
            xSemaphoreGive(xMutex);
        }
        
        // Phản hồi lại client
        String msg = "{\"status\":\"ok\",\"page\":\"neoled\",\"enabled\":" + String(enabled ? "true" : "false") + "}";
        ws.textAll(msg);
    }
    else if (doc["page"] == "fan_control")
    {
        // Điều khiển quạt (Fan control)
        bool enabled = doc["value"]["enabled"].as<bool>();
        
        Serial.printf("🌀 Fan Control: %s\n", enabled ? "BẬT" : "TẮT");
        
        // Cài đặt GPIO làm output và điều khiển
        pinMode(2, OUTPUT);  // GPIO9 for fan (changed from GPIO10 to avoid conflict with flame sensor)
        digitalWrite(2, enabled ? HIGH : LOW);
        
        // Cập nhật biến global (có bảo vệ mutex)
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            glob_fan_enabled = enabled;
            xSemaphoreGive(xMutex);
        }
        
        // Phản hồi lại client
        String msg = "{\"status\":\"ok\",\"page\":\"fan_control\",\"enabled\":" + String(enabled ? "true" : "false") + "}";
        ws.textAll(msg);
    }
}
