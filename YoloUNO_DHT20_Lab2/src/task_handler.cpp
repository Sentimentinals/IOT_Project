#include <task_handler.h>

// Handle incoming WebSocket messages (device control, WiFi/CoreIOT config)
void handleWebSocketMessage(String message)
{
    StaticJsonDocument<256> doc;

    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        return;
    }
    
    JsonObject value = doc["value"];
    
    if (doc["page"] == "device")
    {
        if (!value.containsKey("gpio") || !value.containsKey("status"))
        {
            return;
        }

        int gpio = value["gpio"];
        String status = value["status"].as<String>();

        pinMode(gpio, OUTPUT);
        if (status.equalsIgnoreCase("ON"))
        {
            digitalWrite(gpio, HIGH);
        }
        else if (status.equalsIgnoreCase("OFF"))
        {
            digitalWrite(gpio, LOW);
        }
    }
    else if (doc["page"] == "wifi_setting")
    {
        String ssid = doc["value"]["ssid"].as<String>();
        String pass = doc["value"]["password"].as<String>();

        Serial.println("[Config] WiFi: " + ssid);
        Save_wifi_File(ssid, pass);

        String msg = "{\"status\":\"ok\",\"page\":\"wifi_saved\"}";
        ws.textAll(msg);
    }
    else if (doc["page"] == "coreiot_setting")
    {
        String token = doc["value"]["token"].as<String>();
        String server = doc["value"]["server"].as<String>();
        String port = doc["value"]["port"].as<String>();

        Serial.println("[Config] CoreIOT: " + server);
        Save_coreiot_File(token, server, port);

        String msg = "{\"status\":\"ok\",\"page\":\"coreiot_saved\"}";
        ws.textAll(msg);
    }
}
