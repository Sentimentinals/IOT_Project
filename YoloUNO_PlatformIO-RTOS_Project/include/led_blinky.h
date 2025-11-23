#ifndef __LED_BLINKY__
#define __LED_BLINKY__
#include <Arduino.h>
#include "global.h"

#define LED_GPIO 48  // Normal mode LED
#define LED_ALERT_GPIO 47  // Fire alert LED
#define ENABLE_FADE_MODE


void led_blinky(void *pvParameters);

#endif