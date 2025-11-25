#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

// ==================== SENSOR DATA STRUCTURE ====================
// Thay thế global variables bằng struct truyền qua Queue
typedef struct {
    float temperature;
    float humidity;
    float light_level;
    float moisture_level;
    bool flame_detected;
    bool fan_enabled;
    bool neoled_enabled;
} SensorData_t;

// ==================== SYSTEM STATE ENUM ====================
typedef enum {
    STATE_NORMAL = 0,      // Mọi thứ bình thường
    STATE_WARNING,         // Cảnh báo (temp cao hoặc humidity thấp)
    STATE_CRITICAL,        // Nguy hiểm (temp rất cao hoặc humidity rất thấp)
    STATE_FIRE_ALERT       // Phát hiện cháy - ưu tiên cao nhất
} SystemState_t;

// ==================== WIFI & COREIOT CONFIG ====================
extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern int CORE_IOT_PORT;

// ==================== LEGACY VARIABLES (kept for compatibility) ====================
extern String ssid;
extern String password;
extern String wifi_ssid;
extern String wifi_password;
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

// Sensor data access functions (thread-safe)
void sendSensorData(SensorData_t *data);
bool receiveSensorData(SensorData_t *data, TickType_t timeout);

#endif
