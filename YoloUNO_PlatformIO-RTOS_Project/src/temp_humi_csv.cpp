#include "temp_humi_csv.h"
#include <time.h>

/**
 * CSV DATA EXPORT - Full sensor metrics
 * 
 * Columns:
 * - datetime: Human readable timestamp (YYYY-MM-DD HH:MM:SS)
 * - timestamp: Unix timestamp (seconds)
 * - temperature: Temperature in Celsius
 * - humidity: Humidity percentage
 * - light_level: Light sensor value (0-4095)
 * - moisture_level: Soil moisture percentage (0-100%)
 * - flame_detected: Fire detection (0=No, 1=Yes)
 * - water_pump: Water pump status (0=Off, 1=On)
 * - system_state: Current state (0=Normal, 1=Warning, 2=Critical, 3=Fire)
 */

static String getDateTimeString() {
    if (glob_ntp_synced) {
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        return String(buffer);
    }
    
    // Fallback: use uptime
    unsigned long uptime = millis() / 1000;
    int hours = uptime / 3600;
    int mins = (uptime % 3600) / 60;
    int secs = uptime % 60;
    
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "UP:%02d:%02d:%02d", hours, mins, secs);
    return String(buffer);
}

static unsigned long getTimestamp() {
    if (glob_ntp_synced) {
        time_t now = time(nullptr);
        if (now > 1704067200) {  // Valid timestamp (after 2024)
            return (unsigned long)now;
        }
    }
    return millis() / 1000;
}

static const char* getStateString(SystemState_t state) {
    switch (state) {
        case STATE_NORMAL: return "Normal";
        case STATE_WARNING: return "Warning";
        case STATE_CRITICAL: return "Critical";
        case STATE_FIRE_ALERT: return "Fire";
        default: return "Unknown";
    }
}

void temp_humi_csv(void *pvParameters) {
    Serial.println("[CSV] Task started");
    
    // Create or verify CSV file with proper header
    bool needHeader = !LittleFS.exists(CSV_FILE);
    
    if (!needHeader) {
        // Check if file has correct header
        File checkFile = LittleFS.open(CSV_FILE, "r");
        if (checkFile) {
            String firstLine = checkFile.readStringUntil('\n');
            checkFile.close();
            
            // If header doesn't match new format, recreate file
            if (!firstLine.startsWith("datetime,timestamp")) {
                needHeader = true;
                LittleFS.remove(CSV_FILE);
            }
        }
    }
    
    if (needHeader) {
        File csvFile = LittleFS.open(CSV_FILE, "w");
        if (csvFile) {
            csvFile.println("datetime,timestamp,temperature,humidity,light_level,moisture_level,flame_detected,water_pump,system_state");
            csvFile.close();
            Serial.println("[CSV] Header created");
        }
    }
    
    SensorData_t sensorData = {0};
    
    while (1) {
        // Get sensor data from queue
        if (xSensorDataQueue != NULL) {
            xQueuePeek(xSensorDataQueue, &sensorData, pdMS_TO_TICKS(100));
        }
        
        // Only log if we have valid temperature and humidity
        bool validData = (sensorData.temperature > 0 && sensorData.temperature < 80 &&
                         sensorData.humidity > 0 && sensorData.humidity <= 100);
        
        if (validData) {
            File csvFile = LittleFS.open(CSV_FILE, "a");
            if (csvFile) {
                SystemState_t currentState = getSystemState();
                
                // Format: datetime,timestamp,temp,humidity,light,moisture,flame,water_pump,state
                csvFile.print(getDateTimeString());
                csvFile.print(",");
                csvFile.print(getTimestamp());
                csvFile.print(",");
                csvFile.print(sensorData.temperature, 2);
                csvFile.print(",");
                csvFile.print(sensorData.humidity, 2);
                csvFile.print(",");
                csvFile.print((int)sensorData.light_level);
                csvFile.print(",");
                csvFile.print(sensorData.moisture_level, 1);
                csvFile.print(",");
                csvFile.print(sensorData.flame_detected ? "1" : "0");
                csvFile.print(",");
                csvFile.print(sensorData.water_pump_enabled ? "1" : "0");
                csvFile.print(",");
                csvFile.println(getStateString(currentState));
                
                csvFile.close();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(CSV_INTERVAL_MS));
    }
}
