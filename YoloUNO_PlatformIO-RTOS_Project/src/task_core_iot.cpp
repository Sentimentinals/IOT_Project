
#include "task_core_iot.h"

constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

constexpr char LED_STATE_ATTR[] = "ledState";

volatile int ledMode = 0;
volatile bool ledState = false;

constexpr uint16_t BLINKING_INTERVAL_MS_MIN = 10U;
constexpr uint16_t BLINKING_INTERVAL_MS_MAX = 60000U;
volatile uint16_t blinkingInterval = 1000U;

constexpr int16_t telemetrySendInterval = 10000U;

constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
    LED_STATE_ATTR,
};

void processSharedAttributes(const Shared_Attribute_Data &data)
{
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        // if (strcmp(it->key().c_str(), BLINKING_INTERVAL_ATTR) == 0)
        // {
        //     const uint16_t new_interval = it->value().as<uint16_t>();
        //     if (new_interval >= BLINKING_INTERVAL_MS_MIN && new_interval <= BLINKING_INTERVAL_MS_MAX)
        //     {
        //         blinkingInterval = new_interval;
        //         Serial.print("Blinking interval is set to: ");
        //         Y
        //             Serial.println(new_interval);
        //     }
        // }
        // if (strcmp(it->key().c_str(), LED_STATE_ATTR) == 0)
        // {
        //     ledState = it->value().as<bool>();
        // digitalWrite(LED_PIN, ledState);
        // Serial.print("LED state is set to: ");
        // Serial.println(ledState);
        // }
    }
}

// RPC Callbacks for remote control from CoreIOT
RPC_Response setFanStatus(const RPC_Data &data) {
    Serial.println("🌀 Received Fan control command from CoreIOT");
    bool newState = data;
    
    glob_fan_enabled = newState;  // Update global variable
    
    // Control actual fan hardware (assuming FAN_PIN = 14)
    pinMode(14, OUTPUT);
    digitalWrite(14, newState ? HIGH : LOW);
    
    Serial.printf("Fan turned %s\n", newState ? "ON" : "OFF");
    return RPC_Response("setFanStatus", newState);
}

RPC_Response setLedEnabled(const RPC_Data &data) {
    Serial.println("💡 Received NeoLed control command from CoreIOT");
    bool newState = data;
    
    glob_neoled_enabled = newState;  // Update global variable
    
    Serial.printf("NeoLed %s\n", newState ? "ENABLED" : "DISABLED");
    return RPC_Response("setLedEnabled", newState);
}

// Register RPC callbacks
const std::array<RPC_Callback, 2U> callbacks = {
    RPC_Callback{"setFanStatus", setFanStatus},
    RPC_Callback{"setLedEnabled", setLedEnabled}
};

const Shared_Attribute_Callback attributes_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
const Attribute_Request_Callback attribute_shared_request_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());

void CORE_IOT_sendata(String mode, String feed, String data)
{
    if (mode == "attribute")
    {
        tb.sendAttributeData(feed.c_str(), data);
    }
    else if (mode == "telemetry")
    {
        float value = data.toFloat();
        tb.sendTelemetryData(feed.c_str(), value);
    }
    else
    {
        // handle unknown mode
    }
}

void CORE_IOT_reconnect()
{
    if (!tb.connected())
    {
        if (!tb.connect(CORE_IOT_SERVER.c_str(), CORE_IOT_TOKEN.c_str(), CORE_IOT_PORT))  // CORE_IOT_PORT is already int
        {
            // Serial.println("Failed to connect");
            return;
        }

        tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());

        Serial.println("Subscribing for RPC...");
        if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend()))
        {
            // Serial.println("Failed to subscribe for RPC");
            return;
        }

        if (!tb.Shared_Attributes_Subscribe(attributes_callback))
        {
            // Serial.println("Failed to subscribe for shared attribute updates");
            return;
        }

        Serial.println("Subscribe done");

        if (!tb.Shared_Attributes_Request(attribute_shared_request_callback))
        {
            // Serial.println("Failed to request for shared attributes");
            return;
        }
        tb.sendAttributeData("localIp", WiFi.localIP().toString().c_str());
    }
    else if (tb.connected())
    {
        tb.loop();
    }
}

// CoreIOT Publishing Task
void CORE_IOT_task(void *pvParameters)
{
    Serial.println("🌐 CoreIOT Task Started");
    
    // Wait for WiFi connection
    while (1)
    {
        if (xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY))
        {
            Serial.println("✅ WiFi Connected - Starting CoreIOT");
            break;
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    unsigned long lastPublish = 0;
    const unsigned long PUBLISH_INTERVAL = 10000; // 10 seconds

    while (1)
    {
        CORE_IOT_reconnect();

        unsigned long now = millis();
        if (now - lastPublish >= PUBLISH_INTERVAL)
        {
            lastPublish = now;

            // Publish all sensor telemetry data
            if (tb.connected())
            {
                Serial.println("📤 Publishing sensor data to CoreIOT...");
                
                tb.sendTelemetryData("temperature", glob_temperature);
                tb.sendTelemetryData("humidity", glob_humidity);
                tb.sendTelemetryData("light", glob_light_level);
                tb.sendTelemetryData("moisture", glob_moisture_level);
                tb.sendTelemetryData("flame", glob_flame_detected ? 1 : 0);
                tb.sendTelemetryData("fanStatus", glob_fan_enabled ? 1 : 0);
                tb.sendTelemetryData("neoLedEnabled", glob_neoled_enabled ? 1 : 0);

                Serial.printf("  🌡️  Temp: %.1f°C | 💧 Hum: %.1f%%\n", glob_temperature, glob_humidity);
                Serial.printf("  ☀️  Light: %.0f lux | 🌱 Moisture: %.1f%%\n", glob_light_level, glob_moisture_level);
                Serial.printf("  🔥 Flame: %s | 🌀 Fan: %s\n", 
                    glob_flame_detected ? "DETECTED" : "Safe",
                    glob_fan_enabled ? "ON" : "OFF");
            }
            else
            {
                Serial.println("⚠️  Not connected to CoreIOT");
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}