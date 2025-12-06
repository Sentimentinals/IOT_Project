#ifndef __TEMP_HUMI_CSV_H__
#define __TEMP_HUMI_CSV_H__

#include "global.h"
#include <Arduino.h>
#include "LittleFS.h"

// File CSV để lưu dữ liệu
#define CSV_FILE "/sensor_data.csv"
#define CSV_INTERVAL_MS 5000*60  // Ghi mỗi 5 phút

void temp_humi_csv(void *pvParameters);

#endif

