#include <task_handler.h>
#include "sensor_water_pump.h"

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
    else if (doc["page"] == "neoled")
    {
        bool enabled = doc["value"]["enabled"].as<bool>();
        
        // Thread-safe update
        updateSensorField_NeoLed(enabled);
        
        String msg = "{\"status\":\"ok\",\"page\":\"neoled\",\"enabled\":" + String(enabled ? "true" : "false") + "}";
        ws.textAll(msg);
    }
    else if (doc["page"] == "fan_control")
    {
        bool enabled = doc["value"]["enabled"].as<bool>();
        
        pinMode(2, OUTPUT);
        digitalWrite(2, enabled ? HIGH : LOW);
        
        // Thread-safe update
        updateSensorField_Fan(enabled);
        
        String msg = "{\"status\":\"ok\",\"page\":\"fan_control\",\"enabled\":" + String(enabled ? "true" : "false") + "}";
        ws.textAll(msg);
    }
    else if (doc["page"] == "pump_control")
    {
        bool enabled = doc["value"]["enabled"].as<bool>();
        
        // Use dedicated function for manual pump control
        setWaterPumpManual(enabled);
        
        String msg = "{\"status\":\"ok\",\"page\":\"pump_control\",\"enabled\":" + String(enabled ? "true" : "false") + ",\"mode\":\"manual\"}";
        ws.textAll(msg);
    }
}
