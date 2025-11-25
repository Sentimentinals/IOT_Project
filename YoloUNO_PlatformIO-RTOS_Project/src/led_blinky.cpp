#include "led_blinky.h"

/**
 * LED BLINKY TASK
 * - FIRE MODE: LED_ALERT_GPIO (GPIO47) solid ON
 * - NORMAL MODE: LED_GPIO (GPIO48) blinking
 */

void led_blinky(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);
  pinMode(LED_ALERT_GPIO, OUTPUT);
  
  Serial.println("[LED] Task started");
  
  SensorData_t sensorData = {0};
  bool blinkState = false;
  
  while(1){
    // Read flame status from queue
    bool flameDetected = false;
    if (xSensorDataQueue != NULL) {
      xQueuePeek(xSensorDataQueue, &sensorData, pdMS_TO_TICKS(50));
      flameDetected = sensorData.flame_detected;
    }
    
    // Also check system state
    SystemState_t state = getSystemState();
    if (state == STATE_FIRE_ALERT) {
      flameDetected = true;
    }
    
    if (flameDetected) {
      // FIRE MODE - Alert LED solid ON, normal LED OFF
      analogWrite(LED_ALERT_GPIO, 255);  // Solid bright
      analogWrite(LED_GPIO, 0);
      vTaskDelay(pdMS_TO_TICKS(100));
      
    } else {
      // NORMAL MODE - Alert LED OFF, normal LED blinking
      analogWrite(LED_ALERT_GPIO, 0);
      
      blinkState = !blinkState;
      analogWrite(LED_GPIO, blinkState ? 255 : 0);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}
