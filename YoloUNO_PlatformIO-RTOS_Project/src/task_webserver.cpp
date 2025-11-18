#include "task_webserver.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool webserver_isrunning = false;

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
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
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
    
    server.begin();
    ElegantOTA.begin(&server);
    webserver_isrunning = true;
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
