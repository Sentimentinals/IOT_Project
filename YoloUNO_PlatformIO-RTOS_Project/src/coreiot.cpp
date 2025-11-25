#include "coreiot.h"

// ----------- CONFIGURE THESE! -----------
const char* coreIOT_Server = "app.coreiot.io";  
const char* coreIOT_Token = "g7drm1amhd3dchr379xu";   // Device Access Token
const int   mqttPort = 1883;
// ----------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);


void reconnect() {
  while (!client.connected()) {
    Serial.print("[CoreIOT] Connecting...");
    if (client.connect("ESP32Client", coreIOT_Token, NULL)) {
      Serial.println("connected!");
      client.subscribe("v1/devices/me/rpc/request/+");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retry in 5s");
      delay(5000);
    }
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[CoreIOT] Message [");
  Serial.print(topic);
  Serial.println("]");

  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    return;
  }

  const char* method = doc["method"];
  if (strcmp(method, "setStateLED") == 0) {
    const char* params = doc["params"];

    if (strcmp(params, "ON") == 0) {
      Serial.println("[CoreIOT] LED ON");
    } else {   
      Serial.println("[CoreIOT] LED OFF");
    }
  }
}


void setup_coreiot(){
  while(1){
    if (xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY)) {
      break;
    }
    delay(500);
  }

  Serial.println("[CoreIOT] WiFi ready");

  client.setServer(coreIOT_Server, mqttPort);
  client.setCallback(callback);
}

void coreiot_task(void *pvParameters){
    setup_coreiot();
    
    SensorData_t sensorData = {0};

    while(1){
        if (!client.connected()) {
            reconnect();
        }
        client.loop();

        // Read sensor data from Queue
        if (xSensorDataQueue != NULL) {
            xQueuePeek(xSensorDataQueue, &sensorData, pdMS_TO_TICKS(100));
        }

        // Build payload from queue data
        String payload = "{\"temperature\":" + String(sensorData.temperature) + 
                         ",\"humidity\":" + String(sensorData.humidity) + "}";
        
        client.publish("v1/devices/me/telemetry", payload.c_str());

        vTaskDelay(10000);
    }
}
