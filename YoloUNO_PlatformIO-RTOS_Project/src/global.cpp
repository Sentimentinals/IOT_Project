#include "global.h"

// ==================== SENSOR GLOBALS ====================
float glob_temperature = 0;
float glob_humidity = 0;
float glob_light_level = 0;
float glob_moisture_level = 0;
bool glob_flame_detected = false;
bool glob_fan_enabled = false;
bool glob_neoled_enabled = true;  // NeoPixel LED control (default ON)
bool glob_ntp_synced = false;

// ==================== WIFI CONFIG ====================
// WiFi Credentials (loaded from Settings page or LittleFS)
// Leave empty to use AP mode only ("TaiHieuTien" / "12345678")
String WIFI_SSID = "";  
String WIFI_PASS = "";

// ==================== COREIOT CONFIG ====================
// CoreIOT Credentials (configured via Settings page)
String CORE_IOT_TOKEN = "";
String CORE_IOT_SERVER = "app.coreiot.io";
int CORE_IOT_PORT = 1883;

// ==================== LEGACY VARIABLES ====================
String ssid = "ESP32-YOUR NETWORK HERE!!!";
String password = "12345678";
String wifi_ssid = "";
String wifi_password = "";
boolean isWifiConnected = false;

// ==================== RTOS SEMAPHORES ====================
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();
SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t xI2CMutex = xSemaphoreCreateMutex();  // Mutex for I2C bus