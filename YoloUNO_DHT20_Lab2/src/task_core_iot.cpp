#include "task_core_iot.h"

constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

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

                getSensorData(&sensorData);

                if (tb.connected())
                {
                    tb.sendTelemetryData("temperature", sensorData.temperature);
                    tb.sendTelemetryData("humidity", sensorData.humidity);
                    
                    SystemState_t currentState = getSystemState();
                    tb.sendTelemetryData("systemState", (int)currentState);

                    if (WiFi.status() == WL_CONNECTED)
                    {
                        int32_t rssi = WiFi.RSSI();
                        tb.sendTelemetryData("wifiSignal", (int)rssi);
                    }
                    
                    Serial.printf("[CoreIOT] Published: T=%.1f, H=%.1f\n", 
                                  sensorData.temperature, sensorData.humidity);
                }
            }
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
