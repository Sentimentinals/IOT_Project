#include "task_check_info.h"

void Load_info_File()
{
  File file = LittleFS.open("/info.dat", "r");
  if (!file)
  {
    Serial.println("ℹ️ No config file found - using defaults");
    return;
  }
  
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error)
  {
    Serial.print(F("❌ deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  
  // Đọc an toàn - kiểm tra null trước khi gán
  if (doc.containsKey("WIFI_SSID") && !doc["WIFI_SSID"].isNull()) {
    WIFI_SSID = doc["WIFI_SSID"].as<String>();
  }
  if (doc.containsKey("WIFI_PASS") && !doc["WIFI_PASS"].isNull()) {
    WIFI_PASS = doc["WIFI_PASS"].as<String>();
  }
  if (doc.containsKey("CORE_IOT_TOKEN") && !doc["CORE_IOT_TOKEN"].isNull()) {
    CORE_IOT_TOKEN = doc["CORE_IOT_TOKEN"].as<String>();
  }
  if (doc.containsKey("CORE_IOT_SERVER") && !doc["CORE_IOT_SERVER"].isNull()) {
    CORE_IOT_SERVER = doc["CORE_IOT_SERVER"].as<String>();
  }
  if (doc.containsKey("CORE_IOT_PORT")) {
    CORE_IOT_PORT = doc["CORE_IOT_PORT"].as<int>();
  }
  
  Serial.println("✅ Config loaded successfully");
  Serial.println("📡 WiFi SSID: " + (WIFI_SSID.isEmpty() ? "(empty)" : WIFI_SSID));
}

void Delete_info_File()
{
  if (LittleFS.exists("/info.dat"))
  {
    LittleFS.remove("/info.dat");
  }
  ESP.restart();
}

void Save_info_File(String wifi_ssid, String wifi_pass, String core_iot_token, String core_iot_server, String core_iot_port)
{
  Serial.println(wifi_ssid);
  Serial.println(wifi_pass);

  DynamicJsonDocument doc(4096);
  doc["WIFI_SSID"] = wifi_ssid;
  doc["WIFI_PASS"] = wifi_pass;
  doc["CORE_IOT_TOKEN"] = core_iot_token;
  doc["CORE_IOT_SERVER"] = core_iot_server;
  doc["CORE_IOT_PORT"] = core_iot_port.toInt();

  File configFile = LittleFS.open("/info.dat", "w");
  if (configFile)
  {
    serializeJson(doc, configFile);
    configFile.close();
  }
  else
  {
    Serial.println("Unable to save the configuration.");
  }
  ESP.restart();
}

bool check_info_File(bool check)
{
  static bool wifi_logged = false;
  
  if (!check)
  {
    if (!LittleFS.begin(true))
    {
      Serial.println("❌ Lỗi khởi động LittleFS!");
      return false;
    }
    
    // 🔧 DEBUG: Uncomment dòng dưới để xóa config nếu bị lỗi
    // LittleFS.remove("/info.dat"); Serial.println("🗑️ Config cleared!");
    
    Load_info_File();
    startAP();
    wifi_logged = false;
  }
  
  if (!WIFI_SSID.isEmpty() && !WIFI_PASS.isEmpty())
  {
    if (!wifi_logged) {
      Serial.println("📡 WiFi credentials found - connecting to home WiFi...");
      wifi_logged = true;
    }
    return true;
  }
  
  if (!wifi_logged) {
    Serial.println("ℹ️ No WiFi credentials - AP-only mode");
    wifi_logged = true;
  }
  return false;
}

// Save only WiFi credentials
void Save_wifi_File(String wifi_ssid, String wifi_pass)
{
  DynamicJsonDocument doc(4096);
  
  File file = LittleFS.open("/info.dat", "r");
  if (file) {
    deserializeJson(doc, file);
    file.close();
  }
  
  doc["WIFI_SSID"] = wifi_ssid;
  doc["WIFI_PASS"] = wifi_pass;
  
  File configFile = LittleFS.open("/info.dat", "w");
  if (configFile) {
    serializeJson(doc, configFile);
    configFile.close();
    Serial.println("✅ WiFi config saved");
  } else {
    Serial.println("❌ Failed to save WiFi config");
  }
  ESP.restart();
}

// Save only CoreIOT credentials
void Save_coreiot_File(String core_iot_token, String core_iot_server, String core_iot_port)
{
  DynamicJsonDocument doc(4096);
  
  File file = LittleFS.open("/info.dat", "r");
  if (file) {
    deserializeJson(doc, file);
    file.close();
  }
  
  doc["CORE_IOT_TOKEN"] = core_iot_token;
  doc["CORE_IOT_SERVER"] = core_iot_server;
  doc["CORE_IOT_PORT"] = core_iot_port.toInt();
  
  File configFile = LittleFS.open("/info.dat", "w");
  if (configFile) {
    serializeJson(doc, configFile);
    configFile.close();
    Serial.println("✅ CoreIOT config saved");
  } else {
    Serial.println("❌ Failed to save CoreIOT config");
  }
  ESP.restart();
}