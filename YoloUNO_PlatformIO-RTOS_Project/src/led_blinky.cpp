#include "led_blinky.h"

void led_blinky(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);        // GPIO48 - Normal mode
  pinMode(LED_ALERT_GPIO, OUTPUT);  // GPIO47 - Fire alert
  
  while(1){
    // Check flame detection status
    bool flameDetected = false;
    if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      flameDetected = glob_flame_detected;
      xSemaphoreGive(xMutex);
    }
    
    if (flameDetected) {
      // FIRE DETECTED MODE
      // GPIO47: Fast red blink
      analogWrite(LED_ALERT_GPIO, 255);  // Full brightness
      vTaskDelay(pdMS_TO_TICKS(200));    // 200ms ON
      analogWrite(LED_ALERT_GPIO, 0);    // OFF
      vTaskDelay(pdMS_TO_TICKS(200));    // 200ms OFF
      
      // GPIO48: OFF during fire alert
      analogWrite(LED_GPIO, 0);
      
    } else {
      // NORMAL MODE (Safe)
      // GPIO47: OFF
      analogWrite(LED_ALERT_GPIO, 0);
      
      // GPIO48: Smooth fade
#ifdef ENABLE_FADE_MODE
      for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
        analogWrite(LED_GPIO, dutyCycle);
        vTaskDelay(8 / portTICK_PERIOD_MS); 
      }
      for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
        analogWrite(LED_GPIO, dutyCycle);
        vTaskDelay(8 / portTICK_PERIOD_MS);
      }
      vTaskDelay(500 / portTICK_PERIOD_MS);
#else
      digitalWrite(LED_GPIO, HIGH);
      vTaskDelay(1000);
      digitalWrite(LED_GPIO, LOW);
      vTaskDelay(1000);
#endif
    }
  }
}