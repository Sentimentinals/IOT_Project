#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

typedef struct {
    float temperature;
    float humidity;
    float light_level;
    float moisture_level;
    bool flame_detected;
    bool fan_enabled;
    bool neoled_enabled;
    bool water_pump_enabled;
} SensorData_t;

typedef enum {
    STATE_NORMAL = 0,
    STATE_WARNING,
    STATE_CRITICAL,
    STATE_FIRE_ALERT
} SystemState_t;

#define TEMP_COLD           15.0f
#define TEMP_NORMAL_MIN     15.0f
#define TEMP_NORMAL_MAX     30.0f
#define TEMP_HOT            33.0f
#define TEMP_CRITICAL       40.0f

#define HUMIDITY_CRITICAL_LOW   30.0f
#define HUMIDITY_WARNING_LOW    40.0f
#define HUMIDITY_NORMAL_MIN     50.0f
#define HUMIDITY_NORMAL_MAX     70.0f
#define HUMIDITY_WARNING_HIGH   80.0f

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern int CORE_IOT_PORT;

extern boolean isWifiConnected;
extern bool glob_ntp_synced;

extern QueueHandle_t xSensorDataQueue;

extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern SemaphoreHandle_t xI2CMutex;
extern SemaphoreHandle_t xQueueMutex;
extern SemaphoreHandle_t xSerialMutex;
extern SemaphoreHandle_t xSemaphoreNormal;
extern SemaphoreHandle_t xSemaphoreWarning;
extern SemaphoreHandle_t xSemaphoreCritical;
extern SemaphoreHandle_t xSemaphoreFireAlert;

extern volatile SystemState_t currentSystemState;
extern SemaphoreHandle_t xStateMutex;

void initRTOSPrimitives();
SystemState_t evaluateSystemState(float temp, float humidity, bool flame);
void updateSystemState(SystemState_t newState);
SystemState_t getSystemState();
const char* getWarningReason(float temp, float humidity);

void updateSensorField_Light(float value);
void updateSensorField_Moisture(float value);
void updateSensorField_Flame(bool value);
void updateSensorField_Temperature(float value);
void updateSensorField_Humidity(float value);
void updateSensorField_WaterPump(bool value);
void updateSensorField_NeoLed(bool value);
void updateSensorField_Fan(bool value);
bool getSensorData(SensorData_t *data);

void sendSensorData(SensorData_t *data);
bool receiveSensorData(SensorData_t *data, TickType_t timeout);

#endif
