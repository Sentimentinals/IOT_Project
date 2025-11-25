#ifndef __TASK_CHECK_INFO_H__
#define __TASK_CHECK_INFO_H__

#include <ArduinoJson.h>
#include "LittleFS.h"
#include "global.h"
#include "task_wifi.h"

// Access Point Configuration
#ifndef SSID_AP
#define SSID_AP "TaiHieuTien"
#endif

#ifndef PASS_AP
#define PASS_AP "12345678"
#endif


bool check_info_File(bool check);
void Load_info_File();
void Delete_info_File();
void Save_info_File(String WIFI_SSID, String WIFI_PASS, String CORE_IOT_TOKEN, String CORE_IOT_SERVER, String CORE_IOT_PORT);
void Save_wifi_File(String wifi_ssid, String wifi_pass);
void Save_coreiot_File(String core_iot_token, String core_iot_server, String core_iot_port);

#endif