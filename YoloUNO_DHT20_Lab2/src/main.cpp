#include "global.h"
#include <Wire.h>
#include "task_check_info.h"
#include "task_toogle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"
#include "temp_humi_monitor.h"

#define LAB2_WIFI_SSID "DUC TAI"
#define LAB2_WIFI_PASS "20112004"
#define LAB2_COREIOT_TOKEN "B5vTs2W5M1dbgPnFPLhI"
#define LAB2_COREIOT_SERVER "app.coreiot.io"
#define LAB2_COREIOT_PORT 1883

void setup()
{
  Serial.begin(115200);
  delay(2000);
  
  initRTOSPrimitives();
  Wire.begin(11, 12);
  
  bool hasWifiCreds = check_info_File(0);
  
  if (WIFI_SSID.isEmpty()) {
    WIFI_SSID = LAB2_WIFI_SSID;
    WIFI_PASS = LAB2_WIFI_PASS;
    hasWifiCreds = true;
    Serial.println("[Lab2] Using hardcoded WiFi");
  }
  
  if (CORE_IOT_TOKEN.isEmpty()) {
    CORE_IOT_TOKEN = LAB2_COREIOT_TOKEN;
    CORE_IOT_SERVER = LAB2_COREIOT_SERVER;
    CORE_IOT_PORT = LAB2_COREIOT_PORT;
    Serial.println("[Lab2] Using hardcoded CoreIOT token");
  }
  
  Webserver_reconnect();
  
  if (hasWifiCreds) {
    startSTA();
  }
  
  xTaskCreate(temp_humi_monitor, "DHT20", 4096, NULL, 2, NULL);
  xTaskCreate(CORE_IOT_task, "CoreIOT", 4096, NULL, 2, NULL);
  Serial.println("[Setup] Complete - DHT20 Lab2\n");
}

void loop()
{
  Webserver_reconnect();
  
  static unsigned long lastWifiCheck = 0;
  if (millis() - lastWifiCheck > 30000) {
    lastWifiCheck = millis();
    Wifi_reconnect();
  }
  
  delay(10);
}
