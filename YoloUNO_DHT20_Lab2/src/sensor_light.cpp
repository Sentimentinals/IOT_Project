#include "sensor_light.h" 
#include "task_webserver.h"

#define LUX_MIN 1.0f
#define LUX_MAX 65000.0f
#define ADC_DARK 100
#define ADC_BRIGHT 4000

void sensor_light_task(void *pvParameters) {
    Serial.println("[Light] Task started");

    pinMode(LIGHT_SENSOR_PIN, INPUT);
    
    float smoothedLux = 0;
    const float ALPHA = 0.3f;
    bool firstReading = true;
    
    while (1) {
        int rawValue = analogRead(LIGHT_SENSOR_PIN);
        int constrainedADC = constrain(rawValue, ADC_DARK, ADC_BRIGHT);
        float normalized = (float)(constrainedADC - ADC_DARK) / (ADC_BRIGHT - ADC_DARK);
        float luxEstimate = LUX_MIN * pow(LUX_MAX / LUX_MIN, normalized);
        
        if (firstReading) {
            smoothedLux = luxEstimate;
            firstReading = false;
        } else {
            smoothedLux = ALPHA * luxEstimate + (1.0f - ALPHA) * smoothedLux;
        }
        
        float finalLux = round(smoothedLux);
        updateSensorField_Light(finalLux);

        String jsonString = "";
        StaticJsonDocument<128> doc;
        doc["type"] = "sensor"; 
        doc["light"] = finalLux;
        doc["light_raw"] = rawValue;
        serializeJson(doc, jsonString);
        
        if (jsonString.length() > 0) {
            Webserver_sendata(jsonString); 
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
