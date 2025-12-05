#include "tinyml.h"

// Globals
namespace
{
    tflite::ErrorReporter* error_reporter = nullptr;
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;

    constexpr int kTensorArenaSize = 20 * 1024;    // ESP32 has enough RAM
    uint8_t tensor_arena[kTensorArenaSize];
}

void setupTinyML()
{
    Serial.println("[TinyML] Initializing...");

    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(tiny_ml_model);
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model schema mismatch!");
        return;
    }

    static tflite::AllOpsResolver resolver;

    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);

    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed!");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("[TinyML] Model loaded.");
    Serial.printf("[TinyML] Input size = %d floats\n", input->bytes / 4);
}


void tiny_ml_task(void* pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(5000));
    setupTinyML();
    
    if (interpreter == nullptr || input == nullptr || output == nullptr)
    {
        Serial.println("[TinyML] Model failed to load, task exiting.");
        vTaskDelete(NULL);
        return;
    }

    SensorData_t sensorData = {0};

    while (1)
    {
        // Thread-safe read of sensor data
        if (!getSensorData(&sensorData))
        {
            Serial.println("[TinyML] Failed to get sensor data");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // Validate sensor data before inference
        if (sensorData.temperature <= 0 && sensorData.humidity <= 0)
        {
            // No valid data yet, skip inference
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        // ------------------------------
        // Fill input: 4 floats
        // Order MUST MATCH training data:
        // [ temperature, humidity, light_level, moisture_level ]
        // ------------------------------
        input->data.f[0] = sensorData.temperature;
        input->data.f[1] = sensorData.humidity;
        input->data.f[2] = sensorData.light_level;
        input->data.f[3] = sensorData.moisture_level;

        if (interpreter->Invoke() != kTfLiteOk)
        {
            Serial.println("[TinyML] Invoke failed!");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        float y = output->data.f[0];

        Serial.printf("[TinyML] Input: T=%.1f H=%.1f L=%.1f M=%.1f => Output=%.3f\n",
                      sensorData.temperature, sensorData.humidity,
                      sensorData.light_level, sensorData.moisture_level, y);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
