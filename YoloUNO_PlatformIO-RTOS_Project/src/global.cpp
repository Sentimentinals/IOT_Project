#include "global.h"

// ==================== WIFI CONFIG ====================
String WIFI_SSID = "";  
String WIFI_PASS = "";

// ==================== COREIOT CONFIG ====================
String CORE_IOT_TOKEN = "";
String CORE_IOT_SERVER = "app.coreiot.io";
int CORE_IOT_PORT = 1883;

// ==================== LEGACY VARIABLES ====================
String ssid = "ESP32-YOUR NETWORK HERE!!!";
String password = "12345678";
String wifi_ssid = "";
String wifi_password = "";
boolean isWifiConnected = false;
bool glob_ntp_synced = false;

// ==================== GLOBAL STATE ====================
volatile SystemState_t currentSystemState = STATE_NORMAL;
static SystemState_t lastReportedState = STATE_NORMAL;

// ==================== RTOS QUEUES ====================
QueueHandle_t xSensorDataQueue = NULL;

// ==================== RTOS SEMAPHORES ====================
SemaphoreHandle_t xBinarySemaphoreInternet = NULL;
SemaphoreHandle_t xI2CMutex = NULL;
SemaphoreHandle_t xStateMutex = NULL;

SemaphoreHandle_t xSemaphoreNormal = NULL;
SemaphoreHandle_t xSemaphoreWarning = NULL;
SemaphoreHandle_t xSemaphoreCritical = NULL;
SemaphoreHandle_t xSemaphoreFireAlert = NULL;

// ==================== INITIALIZATION ====================
void initRTOSPrimitives() {
    // Create Queue for sensor data (1 slot with overwrite)
    xSensorDataQueue = xQueueCreate(1, sizeof(SensorData_t));
    if (xSensorDataQueue == NULL) {
        Serial.println("[RTOS] ERROR: Queue creation failed!");
    }
    
    // Create Semaphores
    xBinarySemaphoreInternet = xSemaphoreCreateBinary();
    xI2CMutex = xSemaphoreCreateMutex();
    xStateMutex = xSemaphoreCreateMutex();
    
    xSemaphoreNormal = xSemaphoreCreateBinary();
    xSemaphoreWarning = xSemaphoreCreateBinary();
    xSemaphoreCritical = xSemaphoreCreateBinary();
    xSemaphoreFireAlert = xSemaphoreCreateBinary();
    
    // Verify all created successfully
    if (xBinarySemaphoreInternet == NULL || xI2CMutex == NULL || xStateMutex == NULL) {
        Serial.println("[RTOS] ERROR: Semaphore creation failed!");
    }
    
    // Start in normal state
    if (xSemaphoreNormal != NULL) {
        xSemaphoreGive(xSemaphoreNormal);
    }
    
    // Initialize queue with default data
    SensorData_t initData = {0};
    initData.neoled_enabled = true;
    if (xSensorDataQueue != NULL) {
        xQueueSend(xSensorDataQueue, &initData, 0);
    }
    
    Serial.println("[RTOS] Primitives initialized OK");
}

// ==================== HELPER FUNCTIONS ====================

/**
 * Evaluate system state based on sensor data
 * 
 * TEMPERATURE LEVELS:
 * - Cold: < 20C (Warning)
 * - Cool: 20-24C (Normal)
 * - Ideal: 25-30C (Normal - best)
 * - Hot: > 30C (Warning)
 * - Too Hot: > 35C (Critical)
 * 
 * HUMIDITY LEVELS:
 * - Too Dry: < 30% (Critical)
 * - Dry: 30-40% (Warning)
 * - Ideal: 40-60% (Normal - best)
 * - Acceptable: 60-70% (Normal)
 * - Too Humid: > 70% (Warning - mold risk)
 * 
 * Priority: Fire > Critical > Warning > Normal
 */
SystemState_t evaluateSystemState(float temp, float humidity, bool flame) {
    // Fire has highest priority
    if (flame) {
        return STATE_FIRE_ALERT;
    }
    
    // Critical conditions: Too Hot (>35C) or Extremely Dry (<30%)
    if (temp > 35.0 || humidity < 30.0) {
        return STATE_CRITICAL;
    }
    
    // Warning conditions:
    // - Temperature: Cold (<20C) or Hot (>30C)
    // - Humidity: Dry (<40%) or Too Humid (>70%)
    if (temp < 20.0 || temp > 30.0 || humidity < 40.0 || humidity > 70.0) {
        return STATE_WARNING;
    }
    
    // Normal: Temperature 20-30C and Humidity 40-70%
    return STATE_NORMAL;
}

/**
 * Get warning reason string based on current readings
 */
const char* getWarningReason(float temp, float humidity) {
    // Check critical first
    if (temp > 35.0) return "Too Hot";
    if (humidity < 30.0) return "Too Dry";
    
    // Then warnings
    if (temp > 30.0) return "Hot";
    if (temp < 20.0) return "Cold";
    if (humidity < 40.0) return "Dry";
    if (humidity > 70.0) return "Too Humid";
    
    return "Check Environment";
}

/**
 * Update system state and signal semaphores
 */
void updateSystemState(SystemState_t newState) {
    if (xStateMutex == NULL) return;
    
    if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        SystemState_t oldState = currentSystemState;
        
        if (oldState != newState) {
            currentSystemState = newState;
            
            // Clear all state semaphores
            if (xSemaphoreNormal) xSemaphoreTake(xSemaphoreNormal, 0);
            if (xSemaphoreWarning) xSemaphoreTake(xSemaphoreWarning, 0);
            if (xSemaphoreCritical) xSemaphoreTake(xSemaphoreCritical, 0);
            if (xSemaphoreFireAlert) xSemaphoreTake(xSemaphoreFireAlert, 0);
            
            // Give appropriate semaphore
            switch (newState) {
                case STATE_NORMAL:
                    if (xSemaphoreNormal) xSemaphoreGive(xSemaphoreNormal);
                    break;
                case STATE_WARNING:
                    if (xSemaphoreWarning) xSemaphoreGive(xSemaphoreWarning);
                    Serial.println("[STATE] WARNING");
                    break;
                case STATE_CRITICAL:
                    if (xSemaphoreCritical) xSemaphoreGive(xSemaphoreCritical);
                    Serial.println("[STATE] CRITICAL");
                    break;
                case STATE_FIRE_ALERT:
                    if (xSemaphoreFireAlert) xSemaphoreGive(xSemaphoreFireAlert);
                    Serial.println("[STATE] FIRE ALERT!");
                    break;
            }
            
            lastReportedState = newState;
        }
        
        xSemaphoreGive(xStateMutex);
    }
}

/**
 * Get current system state (thread-safe)
 */
SystemState_t getSystemState() {
    SystemState_t state = STATE_NORMAL;
    if (xStateMutex != NULL && xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        state = currentSystemState;
        xSemaphoreGive(xStateMutex);
    }
    return state;
}

/**
 * Send sensor data to queue (preserves control flags like neoled_enabled, fan_enabled)
 */
void sendSensorData(SensorData_t *data) {
    if (xSensorDataQueue != NULL && data != NULL) {
        // Get current data to preserve control flags
        SensorData_t currentData;
        if (xQueuePeek(xSensorDataQueue, &currentData, 0) == pdTRUE) {
            // Preserve control flags from current queue data
            data->neoled_enabled = currentData.neoled_enabled;
            data->fan_enabled = currentData.fan_enabled;
        }
        xQueueOverwrite(xSensorDataQueue, data);
    }
}

/**
 * Receive sensor data from queue
 */
bool receiveSensorData(SensorData_t *data, TickType_t timeout) {
    if (xSensorDataQueue != NULL && data != NULL) {
        return xQueuePeek(xSensorDataQueue, data, timeout) == pdTRUE;
    }
    return false;
}
