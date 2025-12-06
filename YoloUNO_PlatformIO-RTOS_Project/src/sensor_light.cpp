#include "sensor_light.h" 
#include "task_webserver.h"

/**
 * LIGHT SENSOR with LMV358 Op-Amp + Photoresistor (LDR)
 * 
 * LMV358 is an operational amplifier that amplifies the photoresistor signal.
 * Photoresistors can measure from ~0.1 lux (dark) to 100,000+ lux (direct sunlight)
 * 
 * ESP32 ADC: 12-bit (0-4095) = 0-3.3V
 * 
 * Typical light levels:
 * - Moonlight: 0.1-1 lux
 * - Street light: 10-50 lux
 * - Indoor office: 300-500 lux
 * - Overcast day: 1,000-2,000 lux
 * - Bright sunlight: 10,000-100,000 lux
 * 
 * Using logarithmic scale for better range representation
 */

// Calibration constants (adjust based on your circuit)
#define LUX_MIN 1.0f        // Minimum lux (dark)
#define LUX_MAX 65000.0f    // Maximum lux (bright sunlight)
#define ADC_DARK 100        // ADC value in darkness (adjust based on calibration)
#define ADC_BRIGHT 4000     // ADC value in bright light (adjust based on calibration)

void sensor_light_task(void *pvParameters) {
    Serial.println("[Light] Task started");

    pinMode(LIGHT_SENSOR_PIN, INPUT);
    
    // Smoothing filter
    float smoothedLux = 0;
    const float ALPHA = 0.3f;  // Smoothing factor (0-1, lower = smoother)
    bool firstReading = true;
    
    while (1) {
        int rawValue = analogRead(LIGHT_SENSOR_PIN);
        
        // Constrain ADC value to expected range
        int constrainedADC = constrain(rawValue, ADC_DARK, ADC_BRIGHT);
        
        // Map ADC to logarithmic lux scale
        // Using exponential mapping for better representation of light levels
        float normalized = (float)(constrainedADC - ADC_DARK) / (ADC_BRIGHT - ADC_DARK);
        
        // Exponential mapping: lux = LUX_MIN * (LUX_MAX/LUX_MIN)^normalized
        float luxEstimate = LUX_MIN * pow(LUX_MAX / LUX_MIN, normalized);
        
        // Apply smoothing filter
        if (firstReading) {
            smoothedLux = luxEstimate;
            firstReading = false;
        } else {
            smoothedLux = ALPHA * luxEstimate + (1.0f - ALPHA) * smoothedLux;
        }
        
        // Round to reasonable precision
        float finalLux = round(smoothedLux);

        // Thread-safe update of light_level field only
        updateSensorField_Light(finalLux);

        // Send to WebSocket
        String jsonString = "";
        StaticJsonDocument<128> doc;
        doc["type"] = "sensor"; 
        doc["light"] = finalLux;
        doc["light_raw"] = rawValue;  // Debug: send raw ADC value
        serializeJson(doc, jsonString);
        
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString); 
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
