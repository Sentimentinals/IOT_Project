#include "temp_humi_csv.h"
#include <time.h>

static unsigned long getTimestamp()
{
    if (glob_ntp_synced)
    {
        time_t now = time(nullptr);
        if (now > 0) {
            return (unsigned long)now;
        }
    }
    return millis() / 1000;
}

void temp_humi_csv(void *pvParameters) {
    Serial.println("[CSV] Task started");
    
    if (!LittleFS.exists(CSV_FILE)) {
        File csvFile = LittleFS.open(CSV_FILE, "w");
        if (csvFile) {
            csvFile.println("timestamp,temperature,humidity");
            csvFile.close();
        }
    }
    
    SensorData_t sensorData = {0};
    
    while (1) {
        float temperature = 0;
        float humidity = 0;
        
        if (xSensorDataQueue != NULL) {
            if (xQueuePeek(xSensorDataQueue, &sensorData, pdMS_TO_TICKS(100)) == pdTRUE) {
                temperature = sensorData.temperature;
                humidity = sensorData.humidity;
            }
        }
        
        if (temperature > 0 && humidity > 0) {
            File csvFile = LittleFS.open(CSV_FILE, "a");
            if (csvFile) {
                csvFile.print(getTimestamp());
                csvFile.print(",");
                csvFile.print(temperature, 2);
                csvFile.print(",");
                csvFile.println(humidity, 2);
                csvFile.close();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(CSV_INTERVAL_MS));
    }
}
