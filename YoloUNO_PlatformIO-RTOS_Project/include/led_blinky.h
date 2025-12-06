#ifndef __LED_BLINKY__
#define __LED_BLINKY__
#include <Arduino.h>
#include "global.h"

#define LED_GPIO 48
#define LED_ALERT_GPIO 47
#define ENABLE_FADE_MODE


void led_blinky(void *pvParameters);

#endif