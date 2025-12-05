#include "task_wifi.h"
#include <time.h>

static const char *NTP_SERVER = "pool.ntp.org";
static const long GMT_OFFSET_SEC = 7 * 3600;
static const int DAYLIGHT_OFFSET_SEC = 0;

static unsigned long lastReconnectAttempt = 0;
static const unsigned long RECONNECT_INTERVAL = 30000;  // 30 seconds

// Track AP state
static bool apInitialized = false;

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

// Check if AP is actually running
static bool isAPRunning()
{
    if (WiFi.getMode() == WIFI_OFF || WiFi.getMode() == WIFI_STA) {
        return false;
    }
    // Check if AP IP is valid
    IPAddress apIP = WiFi.softAPIP();
    return (apIP[0] != 0);
}

// Initialize or restore AP
static void ensureAP()
{
    // Configure AP IP BEFORE starting AP
    IPAddress apIP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, gateway, subnet);
    
    // Start AP
    bool success = WiFi.softAP(String(SSID_AP), String(PASS_AP), 1, false, 4);
    
    if (success) {
        Serial.printf("[WiFi] AP Active: %s @ %s\n", 
                      String(SSID_AP).c_str(), 
                      WiFi.softAPIP().toString().c_str());
        apInitialized = true;
    } else {
        Serial.println("[WiFi] AP Start FAILED, retrying...");
        delay(500);
        WiFi.softAP(String(SSID_AP), String(PASS_AP), 1, false, 4);
        apInitialized = WiFi.softAPIP()[0] != 0;
    }
}

void startAP()
{
    Serial.println("[WiFi] Initializing AP+STA mode...");
    
    // Disable WiFi first for clean start
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    // Set AP+STA mode
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    
    // Configure for stability
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);  // Don't save to flash (we manage config ourselves)
    
    // Start AP
    ensureAP();
}

void startSTA()
{
    if (WIFI_SSID.isEmpty())
    {
        Serial.println("[WiFi] No STA credentials, AP-only mode");
        return;
    }

    Serial.printf("[WiFi] Connecting to: %s", WIFI_SSID.c_str());

    // Make sure we're in AP+STA mode
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(100);
    }
    
    // Check and restore AP if needed BEFORE connecting STA
    if (!isAPRunning()) {
        Serial.println("\n[WiFi] AP was down, restoring...");
        ensureAP();
    }

    // Start STA connection
    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    
    int timeout = 30;
    while (WiFi.status() != WL_CONNECTED && timeout > 0)
    {
        delay(1000);
        Serial.print(".");
        timeout--;
        
        // Check AP is still running during connection
        if (timeout % 5 == 0 && !isAPRunning()) {
            ensureAP();
        }
    }
    
    // Verify AP is still running after STA connection
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

bool Wifi_reconnect()
{
    // CRITICAL: Always check and restore AP
    if (!isAPRunning()) {
        Serial.println("[WiFi] AP not running, restoring...");
        
        if (WiFi.getMode() != WIFI_AP_STA) {
            WiFi.mode(WIFI_AP_STA);
            delay(100);
        }
        ensureAP();
    }
    
    // Check if already connected
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
    
    // Mark as disconnected
    if (isWifiConnected) {
        isWifiConnected = false;
        Serial.println("[WiFi] STA Connection lost");
    }
    
    // Rate limit reconnection attempts
    if (millis() - lastReconnectAttempt < RECONNECT_INTERVAL) {
        return false;
    }
    lastReconnectAttempt = millis();
    
    // Try to reconnect if we have credentials
    if (!WIFI_SSID.isEmpty()) {
        Serial.println("[WiFi] Attempting STA reconnect...");
        startSTA();
    }
    
    return WiFi.status() == WL_CONNECTED;
}
