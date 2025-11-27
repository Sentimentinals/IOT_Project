#include "sensor_water_pump.h" 
#include "task_webserver.h"

/**
 * WATER PUMP CONTROL TASK
 * 
 * Mode: AUTO (default) or MANUAL
 * - AUTO: Pump ON when moisture < 30%, OFF when moisture > 60%
 * - MANUAL: User controls via WebSocket, stays in manual until timeout (5 min)
 */

static SemaphoreHandle_t xWaterPumpMutex = NULL;
static bool manualModeActive = false;
static bool manualPumpState = false;
static bool lastPumpState = false;
static unsigned long manualModeTimeout = 0;
static const unsigned long MANUAL_TIMEOUT_MS = 300000;  // 5 minutes

void sensor_water_pump_task(void *pvParameters) {
    Serial.println("[WaterPump] Task started");

    pinMode(WATER_PUMP_PIN, OUTPUT);
    digitalWrite(WATER_PUMP_PIN, LOW);
    
    xWaterPumpMutex = xSemaphoreCreateMutex();
    if (xWaterPumpMutex == NULL) {
        Serial.println("[WaterPump] ERROR: Mutex creation failed!");
    }
    
    bool pumpState = false;
    unsigned long lastStatusSend = 0;
    const unsigned long STATUS_INTERVAL = 2000;
    
    while (1) {
        // Get current moisture level from queue (thread-safe)
        SensorData_t sensorData;
        bool hasData = getSensorData(&sensorData);
        
        if (hasData && xWaterPumpMutex != NULL) {
            if (xSemaphoreTake(xWaterPumpMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                
                // Check manual mode timeout
                if (manualModeActive && millis() > manualModeTimeout) {
                    manualModeActive = false;
                    Serial.println("[WaterPump] Manual timeout - AUTO mode");
                }
                
                if (manualModeActive) {
                    // MANUAL MODE
                    pumpState = manualPumpState;
                } else {
                    // AUTO MODE: Control based on soil moisture
                    float moisture = sensorData.moisture_level;
                    
                    if (moisture < SOIL_DRY_THRESHOLD && !pumpState) {
                        pumpState = true;
                        Serial.printf("[WaterPump] AUTO ON - Moisture: %.1f%%\n", moisture);
                    } 
                    else if (moisture > SOIL_WET_THRESHOLD && pumpState) {
                        pumpState = false;
                        Serial.printf("[WaterPump] AUTO OFF - Moisture: %.1f%%\n", moisture);
                    }
                }
                
                // Apply state to GPIO
                digitalWrite(WATER_PUMP_PIN, pumpState ? HIGH : LOW);
                
                xSemaphoreGive(xWaterPumpMutex);
            }
            
            // Log state change
            if (pumpState != lastPumpState) {
                lastPumpState = pumpState;
                Serial.printf("[WaterPump] State: %s (%s)\n", 
                             pumpState ? "ON" : "OFF",
                             manualModeActive ? "MANUAL" : "AUTO");
            }
        }
        
        // Thread-safe update of water_pump_enabled field
        updateSensorField_WaterPump(pumpState);
        
        // Send status to WebSocket periodically
        unsigned long now = millis();
        if (now - lastStatusSend >= STATUS_INTERVAL) {
            lastStatusSend = now;
            
            String jsonString = "";
            StaticJsonDocument<128> doc;
            doc["type"] = "sensor";
            doc["water_pump"] = pumpState;
            doc["pump_auto"] = !manualModeActive;
            serializeJson(doc, jsonString);
            
            if (jsonString.length() > 0) {
                Webserver_sendata(jsonString);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Called from task_handler when user toggles pump on web
extern "C" void setWaterPumpManual(bool enabled) {
    if (xWaterPumpMutex != NULL && xSemaphoreTake(xWaterPumpMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        manualModeActive = true;
        manualPumpState = enabled;
        manualModeTimeout = millis() + MANUAL_TIMEOUT_MS;
        
        digitalWrite(WATER_PUMP_PIN, enabled ? HIGH : LOW);
        
        Serial.printf("[WaterPump] MANUAL %s\n", enabled ? "ON" : "OFF");
        
        xSemaphoreGive(xWaterPumpMutex);
    }
}
