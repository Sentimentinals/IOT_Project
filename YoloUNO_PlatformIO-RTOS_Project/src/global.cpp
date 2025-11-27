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
SemaphoreHandle_t xQueueMutex = NULL;  // NEW: Mutex for queue access

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
    xQueueMutex = xSemaphoreCreateMutex();  // NEW: Create queue mutex
    
    xSemaphoreNormal = xSemaphoreCreateBinary();
    xSemaphoreWarning = xSemaphoreCreateBinary();
    xSemaphoreCritical = xSemaphoreCreateBinary();
    xSemaphoreFireAlert = xSemaphoreCreateBinary();
    
    // Verify all created successfully
    if (xBinarySemaphoreInternet == NULL || xI2CMutex == NULL || 
        xStateMutex == NULL || xQueueMutex == NULL) {
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

// ==================== THREAD-SAFE SENSOR FIELD UPDATES ====================
// Each function uses mutex to safely update ONE field without race conditions

void updateSensorField_Light(float value) {
    if (xQueueMutex == NULL || xSensorDataQueue == NULL) return;
    
    if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        SensorData_t data;
        if (xQueuePeek(xSensorDataQueue, &data, 0) == pdTRUE) {
            data.light_level = value;
            xQueueOverwrite(xSensorDataQueue, &data);
        }
        xSemaphoreGive(xQueueMutex);
    }
}

void updateSensorField_Moisture(float value) {
    if (xQueueMutex == NULL || xSensorDataQueue == NULL) return;
    
    if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        SensorData_t data;
        if (xQueuePeek(xSensorDataQueue, &data, 0) == pdTRUE) {
            data.moisture_level = value;
            xQueueOverwrite(xSensorDataQueue, &data);
        }
        xSemaphoreGive(xQueueMutex);
    }
}

void updateSensorField_Flame(bool value) {
    if (xQueueMutex == NULL || xSensorDataQueue == NULL) return;
    
    if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        SensorData_t data;
        if (xQueuePeek(xSensorDataQueue, &data, 0) == pdTRUE) {
            data.flame_detected = value;
            xQueueOverwrite(xSensorDataQueue, &data);
        }
        xSemaphoreGive(xQueueMutex);
    }
}

void updateSensorField_Temperature(float value) {
    if (xQueueMutex == NULL || xSensorDataQueue == NULL) return;
    
    if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        SensorData_t data;
        if (xQueuePeek(xSensorDataQueue, &data, 0) == pdTRUE) {
            data.temperature = value;
            xQueueOverwrite(xSensorDataQueue, &data);
        }
        xSemaphoreGive(xQueueMutex);
    }
}

void updateSensorField_Humidity(float value) {
    if (xQueueMutex == NULL || xSensorDataQueue == NULL) return;
    
    if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        SensorData_t data;
        if (xQueuePeek(xSensorDataQueue, &data, 0) == pdTRUE) {
            data.humidity = value;
            xQueueOverwrite(xSensorDataQueue, &data);
        }
        xSemaphoreGive(xQueueMutex);
    }
}

void updateSensorField_WaterPump(bool value) {
    if (xQueueMutex == NULL || xSensorDataQueue == NULL) return;
    
    if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        SensorData_t data;
        if (xQueuePeek(xSensorDataQueue, &data, 0) == pdTRUE) {
            data.water_pump_enabled = value;
            xQueueOverwrite(xSensorDataQueue, &data);
        }
        xSemaphoreGive(xQueueMutex);
    }
}

void updateSensorField_NeoLed(bool value) {
    if (xQueueMutex == NULL || xSensorDataQueue == NULL) return;
    
    if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        SensorData_t data;
        if (xQueuePeek(xSensorDataQueue, &data, 0) == pdTRUE) {
            data.neoled_enabled = value;
            xQueueOverwrite(xSensorDataQueue, &data);
        }
        xSemaphoreGive(xQueueMutex);
    }
}

void updateSensorField_Fan(bool value) {
    if (xQueueMutex == NULL || xSensorDataQueue == NULL) return;
    
    if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        SensorData_t data;
        if (xQueuePeek(xSensorDataQueue, &data, 0) == pdTRUE) {
            data.fan_enabled = value;
            xQueueOverwrite(xSensorDataQueue, &data);
        }
        xSemaphoreGive(xQueueMutex);
    }
}

bool getSensorData(SensorData_t *data) {
    if (xQueueMutex == NULL || xSensorDataQueue == NULL || data == NULL) return false;
    
    bool success = false;
    if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        success = (xQueuePeek(xSensorDataQueue, data, 0) == pdTRUE);
        xSemaphoreGive(xQueueMutex);
    }
    return success;
}

// ==================== HELPER FUNCTIONS ====================

/**
 * Evaluate system state based on sensor data
 */
SystemState_t evaluateSystemState(float temp, float humidity, bool flame) {
    if (flame) {
        return STATE_FIRE_ALERT;
    }
    
    if (temp > 35.0 || humidity < 30.0) {
        return STATE_CRITICAL;
    }
    
    if (temp < 20.0 || temp > 30.0 || humidity < 40.0 || humidity > 70.0) {
        return STATE_WARNING;
    }
    
    return STATE_NORMAL;
}

/**
 * Get warning reason string based on current readings
 */
const char* getWarningReason(float temp, float humidity) {
    if (temp > 35.0) return "Too Hot";
    if (humidity < 30.0) return "Too Dry";
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

// ==================== LEGACY FUNCTIONS (for OLED task) ====================

/**
 * Send sensor data to queue (OLED task uses this for temp/humidity)
 * Now uses mutex protection
 * NOTE: Only updates temp/humidity - control flags (neoled, fan, pump) 
 *       are managed separately by their respective handlers
 */
void sendSensorData(SensorData_t *data) {
    if (data == NULL) return;
    
    // Update temperature and humidity using thread-safe functions
    if (data->temperature > 0) {
        updateSensorField_Temperature(data->temperature);
    }
    if (data->humidity > 0) {
        updateSensorField_Humidity(data->humidity);
    }
    // NOTE: Do NOT update neoled_enabled here - it's controlled by web interface
    // updateSensorField_NeoLed() is called only from task_handler when user toggles
}

/**
 * Receive sensor data from queue
 */
bool receiveSensorData(SensorData_t *data, TickType_t timeout) {
    return getSensorData(data);
}
