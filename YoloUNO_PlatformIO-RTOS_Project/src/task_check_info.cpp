#include "task_check_info.h"

// Load WiFi and CoreIOT configuration from LittleFS
void Load_info_File()
{
  File file = LittleFS.open("/info.dat", "r");
  if (!file) {
    Serial.println("[Config] No config file found");
    return;
  }
  
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.println("[Config] Parse error");
    return;
  }
  
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
  
  Serial.println("[Config] Loaded OK");
  if (!WIFI_SSID.isEmpty()) {
    Serial.printf("[Config] WiFi SSID: %s\n", WIFI_SSID.c_str());
  }
}

// Delete config file and restart device (factory reset)
void Delete_info_File()
{
  if (LittleFS.exists("/info.dat"))
  {
    LittleFS.remove("/info.dat");
  }
  ESP.restart();
}

// Save WiFi and CoreIOT configuration to LittleFS
void Save_info_File(String wifi_ssid, String wifi_pass, String core_iot_token, String core_iot_server, String core_iot_port)
{
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
  ESP.restart();
}

// Check if config file exists and load it, optionally start AP
bool check_info_File(bool check)
{
  static bool initialized = false;
  static bool has_wifi_creds = false;
  
  if (!check)
  {
    if (!LittleFS.begin(true))
    {
      Serial.println("[LittleFS] Init failed!");
      return false;
    }
    Serial.println("[LittleFS] Init OK");
    
    Load_info_File();
    startAP();
    has_wifi_creds = !WIFI_SSID.isEmpty();
    initialized = true;
    
    return has_wifi_creds;
  }
  
  return has_wifi_creds;
}

// Save WiFi credentials to config file
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
    Serial.println("[Config] WiFi saved, restarting...");
  }
  
  delay(500);
  ESP.restart();
}

// Save CoreIOT configuration to config file
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
    Serial.println("[Config] CoreIOT saved, restarting...");
  }
  
  delay(500);
  ESP.restart();
}
