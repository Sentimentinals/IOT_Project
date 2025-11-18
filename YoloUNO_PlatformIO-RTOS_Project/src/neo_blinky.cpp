#include "neo_blinky.h"

void neo_blinky(void *pvParameters){

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    // Set all pixels to off to start
    strip.clear();
    strip.show();

    Serial.println("[NEO_BLINKY] NeoPixel initialized successfully");

    strip.setBrightness(0);
    // colors to show
    uint32_t colors[] = {
        strip.Color(255, 0, 0),   // Red
        strip.Color(0, 255, 0),   // Green
        strip.Color(0, 0, 255),   // Blue
        strip.Color(255, 255, 0), // Yellow
        strip.Color(255, 0, 255), // Magenta
        strip.Color(0, 255, 255),  // Cyan
        strip.Color(255, 255, 255), // White
    };

    while(1) {                          
        for (uint32_t c : colors) {
        // increase brightness from low (0) to high (255)
            for (int b = 0; b <= 32; b += 4) {
                strip.setPixelColor(0, c);
                strip.setBrightness(b); 
                strip.show();
                vTaskDelay(pdMS_TO_TICKS(15));
            }
            // hold at full brightness briefly
            vTaskDelay(pdMS_TO_TICKS(700));
            // set brightness back to 0 quickly before next color (so next color ramps from low)
            strip.setBrightness(0);
            strip.setPixelColor(0, 0);
            strip.show();
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}