#include "led_blinky.h"

void led_blinky(void *pvParameters){
#ifdef ENABLE_FADE_MODE
  while(1){
    for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
        analogWrite(LED_GPIO, dutyCycle);
        vTaskDelay(8 / portTICK_PERIOD_MS); 
    }
    for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
      analogWrite(LED_GPIO, dutyCycle);
      vTaskDelay(8 / portTICK_PERIOD_MS);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);  
  }
#else
  pinMode(LED_GPIO, OUTPUT);
  while(1) {                        
    digitalWrite(LED_GPIO, HIGH);  // turn the LED ON
    vTaskDelay(1000);
    digitalWrite(LED_GPIO, LOW);  // turn the LED OFF
    vTaskDelay(1000);
  }
#endif
}