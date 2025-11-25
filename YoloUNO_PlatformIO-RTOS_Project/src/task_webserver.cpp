#include "task_webserver.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool webserver_isrunning = false;
unsigned long websocket_connect_time = 0;  // Track WebSocket connection time
void Webserver_sendata(String data)
{
    if (ws.count() > 0)
    {
        ws.textAll(data); // Gửi đến tất cả client đang kết nối
        Serial.println("📤 Đã gửi dữ liệu qua WebSocket: " + data);
    }
    else
    {
        Serial.println("⚠️ Không có client WebSocket nào đang kết nối!");
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        websocket_connect_time = millis();
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        websocket_connect_time = 0;
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;

        if (info->opcode == WS_TEXT)
        {
            String message;
            message += String((char *)data).substring(0, len);
            // parseJson(message, true);
            handleWebSocketMessage(message);
        }
    }
}

void connnectWSV()
{
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/index.html", "text/html"); });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/script.js", "application/javascript"); });
    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/styles.css", "text/css"); });
    server.on("/coreiot_logo.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/coreiot_logo.png", "image/png"); });
    
    // 📥 Endpoint download CSV
    server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  if (LittleFS.exists("/sensor_data.csv")) {
                      request->send(LittleFS, "/sensor_data.csv", "text/csv", true);
                      Serial.println("📥 Người dùng đã download file CSV");
                  } else {
                      request->send(404, "text/plain", "File CSV không tồn tại!");
                  }
              });
    
    // 📊 Endpoint lấy thông tin CSV
    server.on("/csv-info", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  String json = "{";
                  if (LittleFS.exists("/sensor_data.csv")) {
                      File file = LittleFS.open("/sensor_data.csv", "r");
                      if (file) {
                          int size = file.size();
                          int lines = 0;
                          while (file.available()) {
                              if (file.read() == '\n') lines++;
                          }
                          file.close();
                          json += "\"exists\":true,\"size\":" + String(size) + ",\"lines\":" + String(lines);
                      } else {
                          json += "\"exists\":false";
                      }
                  } else {
                      json += "\"exists\":false";
                  }
                  json += "}";
                  request->send(200, "application/json", json);
              });
    
    // 🗑️ Endpoint xóa CSV (reset data)
    server.on("/clear", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  if (LittleFS.remove("/sensor_data.csv")) {
                      request->send(200, "text/plain", "✅ Đã xóa file CSV!");
                      Serial.println("🗑️ Đã xóa file CSV");
                  } else {
                      request->send(500, "text/plain", "❌ Lỗi xóa file!");
                  }
              });

    // 📟 Endpoint thông tin hệ thống
    server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  String json = "{";
                  
                  // System info
                  json += "\"chipModel\":\"" + String(ESP.getChipModel()) + "\",";
                  json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
                  unsigned long uptime_seconds = 0;
                  if (websocket_connect_time > 0)
                  {
                      uptime_seconds = (millis() - websocket_connect_time) / 1000;
                  }
                  json += "\"uptime\":" + String(uptime_seconds) + ",";
                  
                  // WiFi info
                  if (WiFi.status() == WL_CONNECTED) {
                      json += "\"wifiSSID\":\"" + WiFi.SSID() + "\",";
                      json += "\"ipAddress\":\"" + WiFi.localIP().toString() + "\",";
                      json += "\"wifiSignal\":" + String(WiFi.RSSI());
                  } else {
                      json += "\"wifiSSID\":\"Not Connected\",";
                      json += "\"ipAddress\":\"" + WiFi.softAPIP().toString() + " (AP)\",";
                      json += "\"wifiSignal\":0";
                  }
                  
                  json += "}";
                  request->send(200, "application/json", json);
              });
    
    // ⚠️ QUAN TRỌNG: ElegantOTA.begin() phải gọi TRƯỚC server.begin()
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);  // Tự động reboot sau khi upload thành công
    
    // Cấu hình ElegantOTA callbacks để debug
    ElegantOTA.onStart([]() {
        Serial.println("\n🔄 ========== OTA UPDATE STARTING ==========");
        Serial.println("⏳ Đang upload, vui lòng đợi...");
        // Đóng tất cả WebSocket connections để giải phóng RAM
        ws.closeAll();
        Serial.printf("💾 Free Heap: %u bytes\n", ESP.getFreeHeap());
    });
    
    ElegantOTA.onProgress([](size_t current, size_t final_size) {
        static int lastPercent = -1;
        int percent = (current * 100) / final_size;
        if (percent != lastPercent && percent % 5 == 0) {
            Serial.printf("📦 OTA: %d%% (%u/%u)\n", percent, current, final_size);
            lastPercent = percent;
        }
    });
    
    ElegantOTA.onEnd([](bool success) {
        if (success) {
            Serial.println("\n✅ ========== OTA UPDATE SUCCESS ==========");
            Serial.println("🔄 Đang khởi động lại...");
        } else {
            Serial.println("\n❌ ========== OTA UPDATE FAILED ==========");
            Serial.printf("💾 Free Heap: %u bytes\n", ESP.getFreeHeap());
        }
    });
    
    server.begin();
    webserver_isrunning = true;
    Serial.println("✅ WebServer + ElegantOTA ready!");
    Serial.printf("📡 OTA URL: http://%s/update\n", WiFi.localIP().toString().c_str());
}

void Webserver_stop()
{
    ws.closeAll();
    server.end();
    webserver_isrunning = false;
}

void Webserver_reconnect()
{
    if (!webserver_isrunning)
    {
        connnectWSV();
    }
    ElegantOTA.loop();
}
