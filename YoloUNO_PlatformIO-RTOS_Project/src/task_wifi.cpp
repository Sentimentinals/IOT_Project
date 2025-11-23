#include "task_wifi.h"
#include <time.h>

static const char *NTP_SERVER = "pool.ntp.org";
static const long GMT_OFFSET_SEC = 7 * 3600;      // Vietnam timezone (UTC+7)
static const int DAYLIGHT_OFFSET_SEC = 0;

static void syncTimeWithNTP()
{
    if (glob_ntp_synced)
    {
        return;
    }

    Serial.println("⏱️ Đang đồng bộ thời gian với NTP...");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER, "time.nist.gov");

    // Đợi NTP sync
    for (int retries = 0; retries < 15; retries++)
    {
        time_t now = time(nullptr);
        if (now > 1704067200)  // > 2024-01-01 00:00:00 UTC
        {
            glob_ntp_synced = true;
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            Serial.printf("✅ NTP synced: %04d-%02d-%02d %02d:%02d:%02d (Epoch: %ld)\n",
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, now);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (!glob_ntp_synced)
    {
        Serial.println("⚠️ Không thể đồng bộ NTP. CSV sẽ dùng timestamp từ boot.");
    }
}

void startAP()
{
    WiFi.mode(WIFI_AP_STA);  // Changed to dual mode
    WiFi.softAP(String(SSID_AP), String(PASS_AP));
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void startSTA()
{
    if (WIFI_SSID.isEmpty())
    {
        vTaskDelete(NULL);
    }

    WiFi.mode(WIFI_AP_STA);  // Changed to dual mode

    if (WIFI_PASS.isEmpty())
    {
        WiFi.begin(WIFI_SSID.c_str());
    }
    else
    {
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    }

    // Keep AP running while connecting to WiFi
    WiFi.softAP(String(SSID_AP), String(PASS_AP));
    
    Serial.println("🌐 Connecting to WiFi...");
    int timeout = 30;  // 30 second timeout
    while (WiFi.status() != WL_CONNECTED && timeout > 0)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        Serial.print(".");
        timeout--;
    }
    
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\n✅ WiFi Connected!");
        Serial.print("📍 STA IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("📍 AP IP: ");
        Serial.println(WiFi.softAPIP());
        //Give a semaphore here
        xSemaphoreGive(xBinarySemaphoreInternet);
        syncTimeWithNTP();
    }
    else
    {
        Serial.println("\n❌ WiFi Connection Failed!");
        Serial.println("⚠️ AP Mode still active at " + WiFi.softAPIP().toString());
    }
}

bool Wifi_reconnect()
{
    const wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED)
    {
        syncTimeWithNTP();
        return true;
    }
    startSTA();
    return false;
}
