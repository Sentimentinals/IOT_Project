#include "global.h"
float glob_temperature = 0;
float glob_humidity = 0;

String WIFI_SSID;
String WIFI_PASS;
String CORE_IOT_TOKEN;
String CORE_IOT_SERVER;
String CORE_IOT_PORT;

String ssid = "ESP32-YOUR NETWORK HERE!!!";
String password = "12345678";
String wifi_ssid = "abcde";
String wifi_password = "123456789";
boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();
SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t xI2CMutex = xSemaphoreCreateMutex(); // Mutex cho I2C bus

// Điều khiển NeoPixel LED (mặc định BẬT)
bool glob_neoled_enabled = true;
bool glob_ntp_synced = false;