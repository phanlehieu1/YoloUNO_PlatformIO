#ifndef __TINY_ML__
#define __TINY_ML__

#include <Arduino.h>

#include "dht_anomaly_model.h"
#include "global.h"

#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

// ============================================================
//  TinyML anomaly detection state — shared with other tasks
// ============================================================
struct TinyMLState {
    bool isInitialized;       // TF Lite engine ready?
    bool hasValidInput;       // Last sensor reading was valid?
    bool isAnomaly;           // Anomaly detected?
    float lastScore;          // Raw anomaly score (0.0–1.0)
    float threshold;          // Active anomaly threshold
    uint32_t lastInferenceMs; // Inference duration in milliseconds
};

void setupTinyML();
void tiny_ml_task(void *pvParameters);

// Thread-safe getter for the latest TinyML state
TinyMLState getTinyMLState();
const char *getTinyMLModelVersion();
float getTinyMLAnomalyThreshold();

#endif