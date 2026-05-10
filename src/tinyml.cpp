#include "tinyml.h"

// ============================================================
//  TinyML — inference engine (uses accessor functions)
// ============================================================

namespace {
tflite::ErrorReporter *&errorReporter() {
    static tflite::ErrorReporter *reporter = nullptr;
    return reporter;
}

const tflite::Model *&modelHandle() {
    static const tflite::Model *model = nullptr;
    return model;
}

tflite::MicroInterpreter *&interpreterHandle() {
    static tflite::MicroInterpreter *interpreter = nullptr;
    return interpreter;
}

TfLiteTensor *&inputTensor() {
    static TfLiteTensor *input = nullptr;
    return input;
}

TfLiteTensor *&outputTensor() {
    static TfLiteTensor *output = nullptr;
    return output;
}

uint8_t *tensorArena() {
    static uint8_t arena[8 * 1024];
    return arena;
}
} // anonymous namespace

void setupTinyML() {
    Serial.println("TensorFlow Lite Init....");
    static tflite::MicroErrorReporter micro_error_reporter;
    errorReporter() = &micro_error_reporter;

    modelHandle() = tflite::GetModel(dht_anomaly_model_tflite);
    if (modelHandle()->version() != TFLITE_SCHEMA_VERSION) {
        errorReporter()->Report("Model provided is schema version %d, not equal to supported version %d.",
                               modelHandle()->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        modelHandle(), resolver, tensorArena(), 8 * 1024, errorReporter());
    interpreterHandle() = &static_interpreter;

    TfLiteStatus allocate_status = interpreterHandle()->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        errorReporter()->Report("AllocateTensors() failed");
        return;
    }

    inputTensor()  = interpreterHandle()->input(0);
    outputTensor() = interpreterHandle()->output(0);

    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
}

void tiny_ml_task(void *pvParameters) {
    setupTinyML();

    while (1) {
        SensorData data = {-1.0f, -1.0f};
        readLatestSensorData(&data, pdMS_TO_TICKS(50));

        inputTensor()->data.f[0] = data.temperature;
        inputTensor()->data.f[1] = data.humidity;

        TfLiteStatus invoke_status = interpreterHandle()->Invoke();
        if (invoke_status != kTfLiteOk) {
            errorReporter()->Report("Invoke failed");
            return;
        }

        float result = outputTensor()->data.f[0];
        Serial.print("Inference result: ");
        Serial.println(result);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}