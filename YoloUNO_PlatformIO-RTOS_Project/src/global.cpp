#include "global.h"

String WIFI_SSID = "";  
String WIFI_PASS = "";
boolean isWifiConnected = false;
bool glob_ntp_synced = false;

String CORE_IOT_TOKEN = "";
String CORE_IOT_SERVER = "app.coreiot.io";
int CORE_IOT_PORT = 1883;

volatile SystemState_t currentSystemState = STATE_NORMAL;
static SystemState_t lastReportedState = STATE_NORMAL;

QueueHandle_t xSensorDataQueue = NULL;

SemaphoreHandle_t xBinarySemaphoreInternet = NULL;
SemaphoreHandle_t xI2CMutex = NULL;
SemaphoreHandle_t xStateMutex = NULL;
SemaphoreHandle_t xQueueMutex = NULL;
SemaphoreHandle_t xSerialMutex = NULL;

SemaphoreHandle_t xSemaphoreNormal = NULL;
SemaphoreHandle_t xSemaphoreWarning = NULL;
SemaphoreHandle_t xSemaphoreCritical = NULL;
SemaphoreHandle_t xSemaphoreFireAlert = NULL;

// Initialize FreeRTOS primitives (queues, semaphores, mutexes)
void initRTOSPrimitives() {
    xSensorDataQueue = xQueueCreate(1, sizeof(SensorData_t));
    if (xSensorDataQueue == NULL) {
        Serial.println("[RTOS] ERROR: Queue creation failed!");
    }
    
    xBinarySemaphoreInternet = xSemaphoreCreateBinary();
    xI2CMutex = xSemaphoreCreateMutex();
    xStateMutex = xSemaphoreCreateMutex();
    xQueueMutex = xSemaphoreCreateMutex();
    xSerialMutex = xSemaphoreCreateMutex(); 
    
    xSemaphoreNormal = xSemaphoreCreateBinary();
    xSemaphoreWarning = xSemaphoreCreateBinary();
    xSemaphoreCritical = xSemaphoreCreateBinary();
    xSemaphoreFireAlert = xSemaphoreCreateBinary();
    
    if (xBinarySemaphoreInternet == NULL || xI2CMutex == NULL || 
        xStateMutex == NULL || xQueueMutex == NULL) {
        Serial.println("[RTOS] ERROR: Semaphore creation failed!");
    }
    
    if (xSemaphoreNormal != NULL) {
        xSemaphoreGive(xSemaphoreNormal);
    }
    
    SensorData_t initData = {0};
    initData.neoled_enabled = true;
    if (xSensorDataQueue != NULL) {
        xQueueSend(xSensorDataQueue, &initData, 0);
    }
    
    Serial.println("[RTOS] Primitives initialized OK");
}

// Thread-safe update of light sensor value
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

// Thread-safe update of moisture sensor value
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

// Thread-safe update of flame detection status
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

// Thread-safe update of fan control state
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

// Thread-safe read of sensor data from queue
bool getSensorData(SensorData_t *data) {
    if (xQueueMutex == NULL || xSensorDataQueue == NULL || data == NULL) return false;
    
    bool success = false;
    if (xSemaphoreTake(xQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        success = (xQueuePeek(xSensorDataQueue, data, 0) == pdTRUE);
        xSemaphoreGive(xQueueMutex);
    }
    return success;
}

// Evaluate system state based on sensor readings
SystemState_t evaluateSystemState(float temp, float humidity, bool flame) {
    if (flame) {
        return STATE_FIRE_ALERT;
    }
    
    if (temp > TEMP_CRITICAL || humidity < HUMIDITY_CRITICAL_LOW) {
        return STATE_CRITICAL;
    }
    
    if (temp < TEMP_NORMAL_MIN || temp > TEMP_HOT || 
        humidity < HUMIDITY_WARNING_LOW || humidity > HUMIDITY_WARNING_HIGH) {
        return STATE_WARNING;
    }
    
    return STATE_NORMAL;
}

// Get human-readable warning reason string
const char* getWarningReason(float temp, float humidity) {
    if (temp > TEMP_CRITICAL) return "Too Hot";
    if (humidity < HUMIDITY_CRITICAL_LOW) return "Too Dry";
    if (temp > TEMP_HOT) return "Hot";
    if (temp < TEMP_COLD) return "Cold";
    if (temp < TEMP_NORMAL_MIN) return "Cool";
    if (humidity < HUMIDITY_WARNING_LOW) return "Dry";
    if (humidity > HUMIDITY_WARNING_HIGH) return "Too Humid";
    return "Check Environment";
}

// Update system state and signal corresponding semaphore
void updateSystemState(SystemState_t newState) {
    if (xStateMutex == NULL) return;
    
    if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        SystemState_t oldState = currentSystemState;
        
        if (oldState != newState) {
            currentSystemState = newState;
            
            if (xSemaphoreNormal) xSemaphoreTake(xSemaphoreNormal, 0);
            if (xSemaphoreWarning) xSemaphoreTake(xSemaphoreWarning, 0);
            if (xSemaphoreCritical) xSemaphoreTake(xSemaphoreCritical, 0);
            if (xSemaphoreFireAlert) xSemaphoreTake(xSemaphoreFireAlert, 0);
            
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

// Get current system state (thread-safe)
SystemState_t getSystemState() {
    SystemState_t state = STATE_NORMAL;
    if (xStateMutex != NULL && xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        state = currentSystemState;
        xSemaphoreGive(xStateMutex);
    }
    return state;
}

// Update temperature and humidity from sensor data
void sendSensorData(SensorData_t *data) {
    if (data == NULL) return;
    
    if (data->temperature > 0) {
        updateSensorField_Temperature(data->temperature);
    }
    if (data->humidity > 0) {
        updateSensorField_Humidity(data->humidity);
    }
}

// Receive sensor data from queue (legacy compatibility)
bool receiveSensorData(SensorData_t *data, TickType_t timeout) {
    return getSensorData(data);
}
