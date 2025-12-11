#include "led_blinky.h"

#define BLINK_SLOW      1000
#define BLINK_NORMAL    500
#define BLINK_FAST      200
#define BLINK_VERY_FAST 100

// FreeRTOS task: Blink LED based on temperature, show fire alert
void led_blinky(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);
  pinMode(LED_ALERT_GPIO, OUTPUT);
  
  Serial.println("[LED] Task started - Temperature-based blinking enabled");
  
  SensorData_t sensorData = {0};
  bool blinkState = false;
  int blinkInterval = BLINK_NORMAL;
  
  while(1){
    if (xSensorDataQueue != NULL) {
      xQueuePeek(xSensorDataQueue, &sensorData, pdMS_TO_TICKS(50));
    }
    
    SystemState_t state = getSystemState();
    bool flameDetected = sensorData.flame_detected || (state == STATE_FIRE_ALERT);
    
    if (flameDetected) {
      analogWrite(LED_ALERT_GPIO, 255);
      analogWrite(LED_GPIO, 0);
      vTaskDelay(pdMS_TO_TICKS(100));
      
    } else {
      analogWrite(LED_ALERT_GPIO, 0);
      
      float temp = sensorData.temperature;
      
      if (temp < TEMP_COLD) {
        blinkInterval = BLINK_SLOW;
      } 
      else if (temp >= TEMP_COLD && temp < TEMP_HOT) {
        blinkInterval = BLINK_NORMAL;
      }
      else if (temp >= TEMP_HOT && temp < TEMP_CRITICAL) {
        blinkInterval = BLINK_FAST;
      }
      else {
        blinkInterval = BLINK_VERY_FAST;
      }
      
      blinkState = !blinkState;
      analogWrite(LED_GPIO, blinkState ? 255 : 0);
      
      vTaskDelay(pdMS_TO_TICKS(blinkInterval));
    }
  }
}
