#include "task_wifi.h"
#include <time.h>

static const char *NTP_SERVER = "pool.ntp.org";
static const long GMT_OFFSET_SEC = 7 * 3600;
static const int DAYLIGHT_OFFSET_SEC = 0;

static unsigned long lastReconnectAttempt = 0;
static const unsigned long RECONNECT_INTERVAL = 30000;

static bool apInitialized = false;

// Synchronize system time with NTP server
static void syncTimeWithNTP()
{
    if (glob_ntp_synced) return;

    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER, "time.nist.gov");

    for (int retries = 0; retries < 15; retries++)
    {
        time_t now = time(nullptr);
        if (now > 1704067200)
        {
            glob_ntp_synced = true;
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            Serial.printf("[NTP] Synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            break;
        }
        delay(1000);
    }
}

// Check if Access Point is running
static bool isAPRunning()
{
    if (WiFi.getMode() == WIFI_OFF || WiFi.getMode() == WIFI_STA) {
        return false;
    }
    // Check if AP IP is valid
    IPAddress apIP = WiFi.softAPIP();
    return (apIP[0] != 0);
}

// Ensure Access Point is running in AP+STA mode
static void ensureAP()
{
    if (WiFi.getMode() != WIFI_AP_STA) {
        Serial.println("[WiFi] Mode check: switching to AP+STA...");
        WiFi.mode(WIFI_AP_STA);
        delay(200);
    }
    
    if (isAPRunning()) {
        IPAddress currentIP = WiFi.softAPIP();
        if (currentIP[0] == 192 && currentIP[1] == 168 && currentIP[2] == 4 && currentIP[3] == 1) {
            apInitialized = true;
            return;
        }
    }
    
    IPAddress apIP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, gateway, subnet);
    delay(100);
    
    bool success = WiFi.softAP(String(SSID_AP), String(PASS_AP), 1, false, 4);
    delay(200);
    
    if (success) {
        IPAddress actualIP = WiFi.softAPIP();
        if (actualIP[0] != 0) {
            Serial.printf("[WiFi] AP Active: %s @ %s\n", 
                          String(SSID_AP).c_str(), 
                          actualIP.toString().c_str());
            apInitialized = true;
        } else {
            Serial.println("[WiFi] AP started but IP invalid");
            apInitialized = false;
        }
    } else {
        Serial.println("[WiFi] AP Start FAILED, retrying...");
        delay(500);
        WiFi.softAP(String(SSID_AP), String(PASS_AP), 1, false, 4);
        delay(200);
        apInitialized = WiFi.softAPIP()[0] != 0;
        if (apInitialized) {
            Serial.printf("[WiFi] AP Active (retry): %s @ %s\n", 
                          String(SSID_AP).c_str(), 
                          WiFi.softAPIP().toString().c_str());
        }
    }
}

// Start Access Point in AP+STA mode
void startAP()
{
    Serial.println("[WiFi] Initializing AP+STA mode...");
    
    WiFiMode_t currentMode = WiFi.getMode();
    
    if (currentMode == WIFI_OFF || currentMode == WIFI_STA) {
        WiFi.disconnect(true);
        delay(100);
        WiFi.mode(WIFI_AP_STA);
        delay(200);
    } else if (currentMode == WIFI_AP_STA) {
        if (isAPRunning()) {
            Serial.println("[WiFi] AP already running in AP+STA mode");
            apInitialized = true;
            return;
        }
    }
    
    WiFi.setAutoReconnect(false);
    WiFi.persistent(false);
    
    ensureAP();
}

// Connect to WiFi network in Station mode (while maintaining AP)
void startSTA()
{
    if (WIFI_SSID.isEmpty())
    {
        Serial.println("[WiFi] No STA credentials, AP-only mode");
        return;
    }

    Serial.printf("[WiFi] Connecting to: %s", WIFI_SSID.c_str());

    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(100);
    }
    
    if (!isAPRunning()) {
        Serial.println("\n[WiFi] AP was down, restoring...");
        ensureAP();
    }

    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    
    int timeout = 30;
    while (WiFi.status() != WL_CONNECTED && timeout > 0)
    {
        delay(1000);
        Serial.print(".");
        timeout--;
        
        if (timeout % 5 == 0 && !isAPRunning()) {
            ensureAP();
        }
    }
    
    if (!isAPRunning()) {
        Serial.println("\n[WiFi] AP lost during STA connect, restoring...");
        ensureAP();
    }
    
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("\n[WiFi] STA Connected: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WiFi] AP still active: %s @ %s\n", 
                      String(SSID_AP).c_str(), 
                      WiFi.softAPIP().toString().c_str());
        
        isWifiConnected = true;
        lastReconnectAttempt = millis();
    
        String wifiNotif = "{\"wifiStatus\":\"connected\",\"ssid\":\"" + WiFi.SSID() + "\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
        Webserver_sendata(wifiNotif);
    
        if (xBinarySemaphoreInternet != NULL) {
            xSemaphoreGive(xBinarySemaphoreInternet);
        }
        syncTimeWithNTP();
    }
    else
    {
        Serial.println("\n[WiFi] STA Connection failed");
        Serial.printf("[WiFi] AP still active: %s @ %s\n", 
                      String(SSID_AP).c_str(), 
                      WiFi.softAPIP().toString().c_str());
        
        isWifiConnected = false;
    
        String wifiNotif = "{\"wifiStatus\":\"failed\"}";
        Webserver_sendata(wifiNotif);
    }
}

// Reconnect WiFi if disconnected, always maintain AP
bool Wifi_reconnect()
{
    if (WiFi.getMode() != WIFI_AP_STA) {
        Serial.println("[WiFi] Mode check: switching to AP+STA...");
        WiFi.mode(WIFI_AP_STA);
        delay(200);
        ensureAP();
    }
    
    if (!isAPRunning()) {
        Serial.println("[WiFi] AP not running, restoring...");
        ensureAP();
    }
    
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!isWifiConnected) {
            isWifiConnected = true;
            Serial.printf("[WiFi] Reconnected: %s\n", WiFi.localIP().toString().c_str());
            if (xBinarySemaphoreInternet != NULL) {
                xSemaphoreGive(xBinarySemaphoreInternet);
            }
        }
        syncTimeWithNTP();
        return true;
    }
    
    if (isWifiConnected) {
        isWifiConnected = false;
        Serial.println("[WiFi] STA Connection lost");
    }
    
    if (millis() - lastReconnectAttempt < RECONNECT_INTERVAL) {
        return false;
    }
    lastReconnectAttempt = millis();
    
    if (!WIFI_SSID.isEmpty()) {
        Serial.println("[WiFi] Attempting STA reconnect...");
        startSTA();
    }
    
    return WiFi.status() == WL_CONNECTED;
}
