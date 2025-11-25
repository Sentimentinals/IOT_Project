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
#include "task_check_info.h"
#include "task_toogle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"

void setup()
{
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n🚀 ESP32 Starting...");
  
  Wire.begin(11, 12);
  check_info_File(0);
  
  xTaskCreate(led_blinky, "LED", 2048, NULL, 2, NULL);
  xTaskCreate(neo_blinky, "NEO", 2048, NULL, 2, NULL);
  xTaskCreate(temp_humi_oled, "OLED", 4096, NULL, 2, NULL);
  xTaskCreate(temp_humi_csv, "CSV", 4096, NULL, 1, NULL);
  xTaskCreate(sensor_light_task, "Light", 2048, NULL, 2, NULL);
  xTaskCreate(sensor_moisture_task, "Moisture", 2048, NULL, 2, NULL);
  xTaskCreate(sensor_flame_task, "Flame", 2048, NULL, 3, NULL);
  xTaskCreate(CORE_IOT_task, "CoreIOT", 4096, NULL, 2, NULL);
  
  Serial.println("🎉 Setup Complete!\n");
}

void loop()
{
  if (check_info_File(1)) {
    if (!Wifi_reconnect()) {
      Webserver_stop();
    }
    // CoreIOT được xử lý trong CORE_IOT_task, không gọi ở đây
  }
  Webserver_reconnect();
  
  // Yield để các task khác chạy và OTA hoạt động ổn định
  delay(10);
}