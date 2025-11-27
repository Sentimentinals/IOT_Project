#include "task_core_iot.h"
#include "sensor_water_pump.h"

constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

constexpr char LED_STATE_ATTR[] = "ledState";

volatile int ledMode = 0;
volatile bool ledState = false;
volatile uint16_t blinkingInterval = 1000U;

constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
    LED_STATE_ATTR,
};

static bool fanEnabled = false;
static bool neoledEnabled = true;
static bool waterPumpEnabled = false;

void processSharedAttributes(const Shared_Attribute_Data &data)
{
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        // Handle shared attributes
    }
}

RPC_Response setFanStatus(const RPC_Data &data) {
    bool newState = data;
    fanEnabled = newState;
    
    pinMode(14, OUTPUT);
    digitalWrite(14, newState ? HIGH : LOW);
    
    // Thread-safe update
    updateSensorField_Fan(newState);
    
    Serial.printf("[CoreIOT] Fan: %s\n", newState ? "ON" : "OFF");
    return RPC_Response("setFanStatus", newState);
}

RPC_Response setLedEnabled(const RPC_Data &data) {
    bool newState = data;
    neoledEnabled = newState;
    
    // Thread-safe update
    updateSensorField_NeoLed(newState);
    
    Serial.printf("[CoreIOT] NeoLed: %s\n", newState ? "ON" : "OFF");
    return RPC_Response("setLedEnabled", newState);
}

RPC_Response setWaterPumpStatus(const RPC_Data &data) {
    bool newState = data;
    waterPumpEnabled = newState;
    
    // Use dedicated function for proper manual mode handling
    setWaterPumpManual(newState);
    
    Serial.printf("[CoreIOT] Water Pump: %s (manual mode)\n", newState ? "ON" : "OFF");
    return RPC_Response("setWaterPumpStatus", newState);
}

const std::array<RPC_Callback, 3U> callbacks = {
    RPC_Callback{"setFanStatus", setFanStatus},
    RPC_Callback{"setLedEnabled", setLedEnabled},
    RPC_Callback{"setWaterPumpStatus", setWaterPumpStatus}
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
}

void CORE_IOT_reconnect()
{
    if (CORE_IOT_TOKEN.isEmpty() || CORE_IOT_SERVER.isEmpty())
    {
        return;
    }
    
    if (!tb.connected())
    {
        if (!tb.connect(CORE_IOT_SERVER.c_str(), CORE_IOT_TOKEN.c_str(), CORE_IOT_PORT))
        {
            return;
        }
        
        Serial.println("[CoreIOT] Connected");
        tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());

        if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend()))
        {
            return;
        }

        if (!tb.Shared_Attributes_Subscribe(attributes_callback))
        {
            return;
        }

        if (!tb.Shared_Attributes_Request(attribute_shared_request_callback))
        {
            return;
        }
        tb.sendAttributeData("localIp", WiFi.localIP().toString().c_str());
    }
    else
    {
        tb.loop();
    }
}

void CORE_IOT_task(void *pvParameters)
{
    Serial.println("[CoreIOT] Task started");
    
    while (1)
    {
        if (xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY))
        {
            Serial.println("[CoreIOT] WiFi ready");
            break;
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    if (CORE_IOT_TOKEN.isEmpty())
    {
        Serial.println("[CoreIOT] Token not configured");
        vTaskDelete(NULL);
        return;
    }

    unsigned long lastPublish = 0;
    const unsigned long PUBLISH_INTERVAL = 10000;
    SensorData_t sensorData = {0};

    while (1)
    {
        if (!CORE_IOT_TOKEN.isEmpty())
        {
            CORE_IOT_reconnect();

            unsigned long now = millis();
            if (now - lastPublish >= PUBLISH_INTERVAL)
            {
                lastPublish = now;

                // Thread-safe read
                getSensorData(&sensorData);

                if (tb.connected())
                {
                    tb.sendTelemetryData("temperature", sensorData.temperature);
                    tb.sendTelemetryData("humidity", sensorData.humidity);
                    tb.sendTelemetryData("light", sensorData.light_level);
                    tb.sendTelemetryData("moisture", sensorData.moisture_level);
                    tb.sendTelemetryData("flame", sensorData.flame_detected ? 1 : 0);
                    tb.sendTelemetryData("fanStatus", sensorData.fan_enabled ? 1 : 0);
                    tb.sendTelemetryData("neoLedEnabled", sensorData.neoled_enabled ? 1 : 0);
                    tb.sendTelemetryData("waterPumpStatus", sensorData.water_pump_enabled ? 1 : 0);
                    tb.sendTelemetryData("systemState", (int)getSystemState());
                }
            }
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
