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
    STATE_NORMAL = 0,      // Mọi thứ bình thường
    STATE_WARNING,         // Cảnh báo (temp cao hoặc humidity thấp)
    STATE_CRITICAL,        // Nguy hiểm (temp rất cao hoặc humidity rất thấp)
    STATE_FIRE_ALERT       // Phát hiện cháy - ưu tiên cao nhất
} SystemState_t;

// ==================== CENTRALIZED THRESHOLDS ====================
// Temperature thresholds (Celsius)
#define TEMP_COLD           15.0f   // Below this = Cold
#define TEMP_NORMAL_MIN     15.0f   // Comfortable range start (Matches Report Normal 15-33)
#define TEMP_NORMAL_MAX     30.0f   // Comfortable range end
#define TEMP_HOT            33.0f   // Above this = Hot (Warning)
#define TEMP_CRITICAL       40.0f   // Above this = Critical

// Humidity thresholds (%)
#define HUMIDITY_CRITICAL_LOW   30.0f   // Below this = Too Dry (Critical)
#define HUMIDITY_WARNING_LOW    40.0f   // Below this = Dry (Warning)
#define HUMIDITY_NORMAL_MIN     50.0f   // Comfortable range start
#define HUMIDITY_NORMAL_MAX     70.0f   // Comfortable range end (Ideal)
#define HUMIDITY_WARNING_HIGH   80.0f   // Above this = Too Humid (Warning)

// ==================== WIFI & COREIOT CONFIG ====================
extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern int CORE_IOT_PORT;

extern boolean isWifiConnected;
extern bool glob_ntp_synced;

// ==================== RTOS QUEUES ====================
// Queue để truyền sensor data giữa các task
extern QueueHandle_t xSensorDataQueue;

// ==================== RTOS SEMAPHORES ====================
// Binary Semaphore cho Internet connection
extern SemaphoreHandle_t xBinarySemaphoreInternet;

// Mutex cho I2C bus (shared giữa OLED và các I2C devices)
extern SemaphoreHandle_t xI2CMutex;

// Mutex cho Queue access (CRITICAL - prevents race conditions)
extern SemaphoreHandle_t xQueueMutex;

// Mutex cho Serial output (prevents interleaving)
extern SemaphoreHandle_t xSerialMutex;

// Binary Semaphores cho System States
extern SemaphoreHandle_t xSemaphoreNormal;
extern SemaphoreHandle_t xSemaphoreWarning;
extern SemaphoreHandle_t xSemaphoreCritical;
extern SemaphoreHandle_t xSemaphoreFireAlert;

// ==================== GLOBAL STATE (protected by semaphore) ====================
extern volatile SystemState_t currentSystemState;
extern SemaphoreHandle_t xStateMutex;

// ==================== HELPER FUNCTIONS ====================
void initRTOSPrimitives();  // Call in setup() before creating tasks
SystemState_t evaluateSystemState(float temp, float humidity, bool flame);
void updateSystemState(SystemState_t newState);
SystemState_t getSystemState();
const char* getWarningReason(float temp, float humidity);

// Thread-safe sensor data access functions
void updateSensorField_Light(float value);
void updateSensorField_Moisture(float value);
void updateSensorField_Flame(bool value);
void updateSensorField_Temperature(float value);
void updateSensorField_Humidity(float value);
void updateSensorField_WaterPump(bool value);
void updateSensorField_NeoLed(bool value);
void updateSensorField_Fan(bool value);
bool getSensorData(SensorData_t *data);

// Legacy functions (for OLED task compatibility)
void sendSensorData(SensorData_t *data);
bool receiveSensorData(SensorData_t *data, TickType_t timeout);

#endif
