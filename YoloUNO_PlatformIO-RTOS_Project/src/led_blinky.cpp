#include "led_blinky.h"


// Temperature thresholds
#define TEMP_COLD_THRESHOLD     15.0f
#define TEMP_HOT_THRESHOLD      35.0f
#define TEMP_CRITICAL_THRESHOLD 46.0f

// Blink intervals (ms)
#define BLINK_SLOW      1000    // Cold
#define BLINK_NORMAL    500     // Normal
#define BLINK_FAST      200     // Hot
#define BLINK_VERY_FAST 100     // Critical

void led_blinky(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);
  pinMode(LED_ALERT_GPIO, OUTPUT);
  
  Serial.println("[LED] Task started - Temperature-based blinking enabled");
  
  SensorData_t sensorData = {0};
  bool blinkState = false;
  int blinkInterval = BLINK_NORMAL;  // Default interval
  
  while(1){
    // Read sensor data from queue
    if (xSensorDataQueue != NULL) {
      xQueuePeek(xSensorDataQueue, &sensorData, pdMS_TO_TICKS(50));
    }
    
    // Check system state for fire alert
    SystemState_t state = getSystemState();
    bool flameDetected = sensorData.flame_detected || (state == STATE_FIRE_ALERT);
    
    if (flameDetected) {
      analogWrite(LED_ALERT_GPIO, 255);
      analogWrite(LED_GPIO, 0);
      vTaskDelay(pdMS_TO_TICKS(100));
      
    } else {
      analogWrite(LED_ALERT_GPIO, 0);
      
      // Determine blink interval based on temperature
      float temp = sensorData.temperature;
      
      if (temp < TEMP_COLD_THRESHOLD) {
        // COLD: Slow blink
        blinkInterval = BLINK_SLOW;
      } 
      else if (temp >= TEMP_COLD_THRESHOLD && temp < TEMP_HOT_THRESHOLD) {
        // NORMAL: Medium blink
        blinkInterval = BLINK_NORMAL;
      }
      else if (temp >= TEMP_HOT_THRESHOLD && temp < TEMP_CRITICAL_THRESHOLD) {
        // HOT: Fast blink
        blinkInterval = BLINK_FAST;
      }
      else {
        // CRITICAL (>= 35°C): Very fast blink
        blinkInterval = BLINK_VERY_FAST;
      }
      
      // Toggle LED state
      blinkState = !blinkState;
      analogWrite(LED_GPIO, blinkState ? 255 : 0);
      
      // Use temperature-based delay
      vTaskDelay(pdMS_TO_TICKS(blinkInterval));
    }
  }
}
