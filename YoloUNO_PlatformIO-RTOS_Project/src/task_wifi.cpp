#include "task_wifi.h"
#include <time.h>

static const char *NTP_SERVER = "pool.ntp.org";
static const long GMT_OFFSET_SEC = 7 * 3600;
static const int DAYLIGHT_OFFSET_SEC = 0;

static unsigned long lastReconnectAttempt = 0;
static const unsigned long RECONNECT_INTERVAL = 30000;  // 30 seconds

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
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void startAP()
{
    // Always start AP mode first
    WiFi.mode(WIFI_AP_STA);
    
    // Configure WiFi for better stability
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    
    bool apStarted = WiFi.softAP(String(SSID_AP), String(PASS_AP));
    if (apStarted) {
        Serial.printf("[WiFi] AP Started: %s\n", String(SSID_AP).c_str());
        Serial.printf("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("[WiFi] AP Start FAILED!");
    }
}

void startSTA()
{
    // Don't try STA if no credentials
    if (WIFI_SSID.isEmpty())
    {
        Serial.println("[WiFi] No STA credentials, AP-only mode");
        return;
    }

    Serial.printf("[WiFi] Connecting to: %s", WIFI_SSID.c_str());

    // Disconnect first if needed
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    // Start STA connection (AP should already be running)
    if (WIFI_PASS.isEmpty())
    {
        WiFi.begin(WIFI_SSID.c_str());
    }
    else
    {
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    }
    
    int timeout = 30;
    while (WiFi.status() != WL_CONNECTED && timeout > 0)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        Serial.print(".");
        timeout--;
    }
    
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("\n[WiFi] STA Connected: %s\n", WiFi.localIP().toString().c_str());
        isWifiConnected = true;
        lastReconnectAttempt = millis();
    
        String wifiNotif = "{\"wifiStatus\":\"connected\",\"ssid\":\"" + WiFi.SSID() + "\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
        Webserver_sendata(wifiNotif);
    
        // Signal that internet is available
        if (xBinarySemaphoreInternet != NULL) {
            xSemaphoreGive(xBinarySemaphoreInternet);
        }
        syncTimeWithNTP();
    }
    else
    {
        Serial.println("\n[WiFi] STA Connection failed, AP still active");
        isWifiConnected = false;
    
        String wifiNotif = "{\"wifiStatus\":\"failed\"}";
        Webserver_sendata(wifiNotif);
    }
}

bool Wifi_reconnect()
{
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
        Serial.println("[WiFi] Connection lost");
    }
    
    // Rate limit reconnection attempts
    if (millis() - lastReconnectAttempt < RECONNECT_INTERVAL) {
        return false;
    }
    lastReconnectAttempt = millis();
    
    // Try to reconnect if we have credentials
    if (!WIFI_SSID.isEmpty()) {
        Serial.println("[WiFi] Attempting reconnect...");
        startSTA();
    }
    
    return WiFi.status() == WL_CONNECTED;
}
