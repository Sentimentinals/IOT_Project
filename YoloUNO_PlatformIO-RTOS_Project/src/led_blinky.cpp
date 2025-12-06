#include "led_blinky.h"


// Blink intervals (ms)
#define BLINK_SLOW      1000    // Cold (< 15C)
#define BLINK_NORMAL    500     // Normal (15-33C)
#define BLINK_FAST      200     // Hot (33-40C)
#define BLINK_VERY_FAST 100     // Critical (> 40C)

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
      
      // Determine blink interval based on temperature (using centralized thresholds)
      float temp = sensorData.temperature;
      
      if (temp < TEMP_COLD) {
        // COLD: Slow blink
        blinkInterval = BLINK_SLOW;
      } 
      else if (temp >= TEMP_COLD && temp < TEMP_HOT) {
        // NORMAL: Medium blink
        blinkInterval = BLINK_NORMAL;
      }
      else if (temp >= TEMP_HOT && temp < TEMP_CRITICAL) {
        // HOT: Fast blink
        blinkInterval = BLINK_FAST;
      }
      else {
        // CRITICAL: Very fast blink
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
