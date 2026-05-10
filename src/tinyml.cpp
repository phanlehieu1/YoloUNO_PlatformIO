#include "tinyml.h"
#include <ArduinoJson.h>
#include "task_webserver.h"

// ============================================================
//  Task 5: TinyML — Anomaly detection on DHT sensor data
//
//  Uses a pre-trained TFLite model (dht_anomaly_model) that
//  takes [temperature, humidity] as input and outputs an
//  anomaly score (0.0 = normal, 1.0 = anomaly).
//
//  If score >= kAnomalyThreshold → anomaly detected.
//  Results are published to Serial + WebSocket dashboard.
//
//  Architecture: accessor functions (no global variables).
// ============================================================

namespace {

// --- TF Lite engine state (file-scoped, accessor pattern) ---
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

constexpr int kTensorArenaSize = 8 * 1024;
constexpr float kAnomalyThreshold = 0.5f;

uint8_t *tensorArena() {
    static uint8_t arena[kTensorArenaSize];
    return arena;
}

// --- TinyML state (shared with other tasks via getTinyMLState) ---
TinyMLState gTinyMLState = {
    false,   // isInitialized
    false,   // hasValidInput
    false,   // isAnomaly
    0.0f,    // lastScore
    0        // lastInferenceMs
};

void updateTinyMLState(bool isInit, bool validInput, bool anomaly, float score, uint32_t inferMs) {
    gTinyMLState.isInitialized   = isInit;
    gTinyMLState.hasValidInput   = validInput;
    gTinyMLState.isAnomaly       = anomaly;
    gTinyMLState.lastScore       = score;
    gTinyMLState.lastInferenceMs = inferMs;
}

bool hasValidSensorInput(const SensorData &sensorData) {
    return !isnan(sensorData.temperature) && !isnan(sensorData.humidity) &&
           sensorData.temperature >= 0.0f && sensorData.humidity >= 0.0f;
}

} // anonymous namespace

// ============================================================
//  Public API
// ============================================================

TinyMLState getTinyMLState() {
    return gTinyMLState;
}

void setupTinyML() {
    Serial.println("[TinyML] TensorFlow Lite Init....");
    static tflite::MicroErrorReporter micro_error_reporter;
    errorReporter() = &micro_error_reporter;

    modelHandle() = tflite::GetModel(dht_anomaly_model_tflite);
    if (modelHandle()->version() != TFLITE_SCHEMA_VERSION) {
        errorReporter()->Report("Model schema version %d != supported %d.",
                               modelHandle()->version(), TFLITE_SCHEMA_VERSION);
        updateTinyMLState(false, false, false, 0.0f, 0);
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        modelHandle(), resolver, tensorArena(), kTensorArenaSize, errorReporter());
    interpreterHandle() = &static_interpreter;

    if (interpreterHandle()->AllocateTensors() != kTfLiteOk) {
        errorReporter()->Report("AllocateTensors() failed");
        updateTinyMLState(false, false, false, 0.0f, 0);
        return;
    }

    inputTensor()  = interpreterHandle()->input(0);
    outputTensor() = interpreterHandle()->output(0);

    if (!inputTensor() || !outputTensor()) {
        errorReporter()->Report("Tensor pointers unavailable");
        updateTinyMLState(false, false, false, 0.0f, 0);
        return;
    }

    updateTinyMLState(true, false, false, 0.0f, 0);
    Serial.println("[TinyML] TensorFlow Lite Micro initialized on ESP32.");
}

// ============================================================
//  RTOS Task — runs inference every 5 seconds
// ============================================================

void tiny_ml_task(void *pvParameters) {
    setupTinyML();

    SensorData sensorData = {0.0f, 0.0f};

    while (1) {
        // Safety: if engine failed to initialise, keep retrying
        if (!interpreterHandle() || !inputTensor() || !outputTensor()) {
            updateTinyMLState(false, false, false, 0.0f, 0);
            Serial.println("[TinyML] Engine not ready. Retrying in 5s...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // Read latest sensor data via RTOS accessor (thread-safe)
        readLatestSensorData(&sensorData, pdMS_TO_TICKS(100));

        // Validate sensor input before inference
        if (!hasValidSensorInput(sensorData)) {
            updateTinyMLState(true, false, false, 0.0f, 0);
            Serial.println("[TinyML] Invalid sensor input. Skip inference.");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        // Feed sensor data into the model
        inputTensor()->data.f[0] = sensorData.temperature;
        inputTensor()->data.f[1] = sensorData.humidity;

        // Run inference and measure time
        const uint32_t inferenceStartMs = millis();
        if (interpreterHandle()->Invoke() != kTfLiteOk) {
            errorReporter()->Report("Invoke failed");
            updateTinyMLState(true, false, false, 0.0f, 0);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        const float result = outputTensor()->data.f[0];
        const bool isAnomaly = result >= kAnomalyThreshold;
        const uint32_t inferenceElapsedMs = millis() - inferenceStartMs;

        updateTinyMLState(true, true, isAnomaly, result, inferenceElapsedMs);

        // Log to Serial
        Serial.printf("[TinyML] T=%.2f H=%.2f score=%.4f anomaly=%s infer=%lums\n",
                      sensorData.temperature,
                      sensorData.humidity,
                      result,
                      isAnomaly ? "YES ⚠️" : "NO ✅",
                      inferenceElapsedMs);

        // Send anomaly status to WebSocket Dashboard
        StaticJsonDocument<128> doc;
        doc["tinyml_score"] = result;
        doc["tinyml_anomaly"] = isAnomaly;
        doc["tinyml_infer_ms"] = inferenceElapsedMs;

        String jsonString;
        serializeJson(doc, jsonString);
        Webserver_sendata(jsonString);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}