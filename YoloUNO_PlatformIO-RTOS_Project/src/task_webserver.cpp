#include "task_webserver.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool webserver_isrunning = false;
unsigned long websocket_connect_time = 0;
static unsigned long lastCleanup = 0;
static const unsigned long CLEANUP_INTERVAL = 5000;  // Clean up every 5 seconds

void Webserver_sendata(String data)
{
    if (ws.count() > 0)
    {
        ws.textAll(data);
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        websocket_connect_time = millis();
        Serial.printf("[WS] Client #%u connected\n", client->id());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        websocket_connect_time = 0;
        Serial.printf("[WS] Client #%u disconnected\n", client->id());
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->opcode == WS_TEXT)
        {
            String message;
            message += String((char *)data).substring(0, len);
            handleWebSocketMessage(message);
        }
    }
    else if (type == WS_EVT_ERROR)
    {
        Serial.printf("[WS] Client #%u error\n", client->id());
    }
}

void connnectWSV()
{
    // Clean up any existing connections
    ws.closeAll();
    
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    
    // Serve static files with cache headers for better performance
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  request->send(LittleFS, "/index.html", "text/html"); 
              });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/script.js", "application/javascript");
                  response->addHeader("Cache-Control", "no-cache");
                  request->send(response);
              });
    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/styles.css", "text/css");
                  response->addHeader("Cache-Control", "no-cache");
                  request->send(response);
              });
    server.on("/coreiot_logo.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/coreiot_logo.png", "image/png"); });
    
    // Health check endpoint for stability monitoring
    server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  request->send(200, "text/plain", "OK");
              });
    
    // Download CSV endpoint with proper filename
    server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  if (LittleFS.exists("/sensor_data.csv")) {
                      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/sensor_data.csv", "text/csv");
                      response->addHeader("Content-Disposition", "attachment; filename=\"sensor_data.csv\"");
                      request->send(response);
                  } else {
                      request->send(404, "text/plain", "CSV file not found");
                  }
              });
    
    // CSV info endpoint with more details
    server.on("/csv-info", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  String json = "{";
                  if (LittleFS.exists("/sensor_data.csv")) {
                      File file = LittleFS.open("/sensor_data.csv", "r");
                      if (file) {
                          size_t size = file.size();
                          int lines = 0;
                          
                          // Count lines efficiently
                          while (file.available()) {
                              if (file.read() == '\n') lines++;
                          }
                          file.close();
                          
                          json += "\"exists\":true";
                          json += ",\"size\":" + String(size);
                          json += ",\"lines\":" + String(lines);
                          json += ",\"records\":" + String(lines > 0 ? lines - 1 : 0);  // Subtract header
                      } else {
                          json += "\"exists\":false,\"size\":0,\"lines\":0,\"records\":0";
                      }
                  } else {
                      json += "\"exists\":false,\"size\":0,\"lines\":0,\"records\":0";
                  }
                  json += "}";
                  request->send(200, "application/json", json);
              });
    
    // Clear CSV endpoint
    server.on("/clear", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  if (LittleFS.remove("/sensor_data.csv")) {
                      request->send(200, "text/plain", "CSV file cleared");
                  } else {
                      request->send(500, "text/plain", "Error clearing file");
                  }
              });

    // System info endpoint
    server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  String json = "{";
                  
                  size_t freeRam = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
                  json += "\"chipModel\":\"" + String(ESP.getChipModel()) + "\",";
                  json += "\"freeRam\":" + String(freeRam) + ",";
                  unsigned long uptime_seconds = 0;
                  if (websocket_connect_time > 0)
                  {
                      uptime_seconds = (millis() - websocket_connect_time) / 1000;
                  }
                  json += "\"uptime\":" + String(uptime_seconds) + ",";
                  
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
    
    // ElegantOTA setup - MUST be before server.begin()
    // Supports both Firmware and Filesystem (LittleFS) updates
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);
    
    ElegantOTA.onStart([]() {
        Serial.println("\n[OTA] Update starting...");
        
        // Notify clients about OTA update
        String otaNotif = "{\"type\":\"ota\",\"status\":\"starting\"}";
        ws.textAll(otaNotif);
        
        // Close all websocket connections before OTA
        vTaskDelay(pdMS_TO_TICKS(200));
        ws.closeAll();
        vTaskDelay(pdMS_TO_TICKS(100));
    });
    
    ElegantOTA.onProgress([](size_t current, size_t final_size) {
        static int lastPercent = -1;
        int percent = (current * 100) / final_size;
        if (percent != lastPercent && percent % 10 == 0) {
            Serial.printf("[OTA] %d%%\n", percent);
            lastPercent = percent;
        }
    });
    
    ElegantOTA.onEnd([](bool success) {
        if (success) {
            Serial.println("[OTA] Success! Rebooting in 2s...");
            vTaskDelay(pdMS_TO_TICKS(2000));  // Give time for response
        } else {
            Serial.println("[OTA] Failed");
        }
    });
    
    server.begin();
    webserver_isrunning = true;
    
    Serial.println("[WebServer] Started");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[Web] http://%s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[OTA] http://%s/update\n", WiFi.localIP().toString().c_str());
    }
    Serial.printf("[Web] http://%s (AP)\n", WiFi.softAPIP().toString().c_str());
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
    
    // Periodic cleanup of stale WebSocket connections
    if (millis() - lastCleanup >= CLEANUP_INTERVAL) {
        ws.cleanupClients();
        lastCleanup = millis();
    }
    
    // Process OTA
    ElegantOTA.loop();
}
