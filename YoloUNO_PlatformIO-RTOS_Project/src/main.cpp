#include "global.h"
#include <Wire.h>
#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "temp_humi_oled.h"
#include "temp_humi_csv.h"
#include "sensor_light.h"
#include "sensor_moisture.h"
#include "sensor_flame.h"
#include "sensor_water_pump.h"
#include "task_check_info.h"
#include "task_toogle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"

void setup()
{
  Serial.begin(115200);
  delay(2000);
  
  // Initialize RTOS primitives FIRST (before anything else)
  initRTOSPrimitives();
  
  // Initialize I2C
  Wire.begin(11, 12);
  
  // Initialize LittleFS and load config, start AP
  bool hasWifiCreds = check_info_File(0);
  
  // Start webserver immediately (works on AP)
  Webserver_reconnect();
  
  // If we have WiFi credentials, try to connect
  if (hasWifiCreds) {
    startSTA();
  }
  
  // Create tasks (higher number = higher priority)
  xTaskCreate(led_blinky, "LED", 2048, NULL, 1, NULL);
  xTaskCreate(neo_blinky, "NEO", 3072, NULL, 3, NULL);
  xTaskCreate(temp_humi_oled, "OLED", 4096, NULL, 3, NULL);
  xTaskCreate(temp_humi_csv, "CSV", 4096, NULL, 1, NULL);
  xTaskCreate(sensor_light_task, "Light", 2048, NULL, 2, NULL);
  xTaskCreate(sensor_moisture_task, "Moisture", 2048, NULL, 2, NULL);
  xTaskCreate(sensor_flame_task, "Flame", 3072, NULL, 4, NULL);  // Highest - safety
  xTaskCreate(sensor_water_pump_task, "WaterPump", 3072, NULL, 2, NULL);  // Auto irrigation
  xTaskCreate(CORE_IOT_task, "CoreIOT", 4096, NULL, 2, NULL);
  
  Serial.println("[Setup] Complete\n");
}

void loop()
{
  // Keep webserver running
  Webserver_reconnect();
  
  // Check WiFi status periodically
  static unsigned long lastWifiCheck = 0;
  if (millis() - lastWifiCheck > 30000) {  // Every 30 seconds
    lastWifiCheck = millis();
    
    if (check_info_File(1)) {  // If we have credentials
      if (WiFi.status() != WL_CONNECTED) {
        Wifi_reconnect();
      }
    }
  }
  
  delay(10);
}
