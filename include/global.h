#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

// ============================================================
//  Shared data structures (no global variables — RTOS only)
// ============================================================

/** Sensor reading passed through a FreeRTOS queue. */
struct SensorData {
  float temperature;
  float humidity;
};

/** WiFi / CoreIOT credentials stored behind a mutex. */
struct DeviceConfig {
  String wifiSsid;
  String wifiPass;
  String coreIotToken;
  String coreIotServer;
  String coreIotPort;
};

// ============================================================
//  Temperature condition enum (Task 1 — LED blink speed)
// ============================================================
enum TemperatureCondition {
  TEMP_CONDITION_COLD,
  TEMP_CONDITION_NORMAL,
  TEMP_CONDITION_HOT,
  TEMP_CONDITION_SENSOR_ERROR
};

// ============================================================
//  Display state enum (Task 3 — LCD states)
// ============================================================
enum DisplayState {
  STATE_NORMAL = 0,
  STATE_WARNING = 1,
  STATE_CRITICAL = 2
};

// ============================================================
//  RTOS resource initialiser (call once in setup())
// ============================================================
void initRtosResources();

// ============================================================
//  Accessor functions — replace every global variable
// ============================================================

// --- Sensor Queue & Mutex ---
QueueHandle_t  sensorQueue();
SemaphoreHandle_t sensorDataMutex();

// --- Notification semaphores (binary) ---
SemaphoreHandle_t newTemperatureSemaphore();   // Task 1: LED checks this
SemaphoreHandle_t newHumiditySemaphore();       // Task 2: NeoPixel checks this

// --- LCD state semaphores (Task 3) ---
SemaphoreHandle_t normalStateSemaphore();
SemaphoreHandle_t warningStateSemaphore();
SemaphoreHandle_t criticalStateSemaphore();

// --- Internet ---
SemaphoreHandle_t internetConnectedSemaphore();

// --- Device config (mutex-protected) ---
SemaphoreHandle_t deviceConfigMutex();
DeviceConfig getDeviceConfig(TickType_t waitTicks = portMAX_DELAY);
void         setDeviceConfig(const DeviceConfig &config, TickType_t waitTicks = portMAX_DELAY);
bool         hasWifiCredentials();

// --- Sensor data helpers ---
bool readLatestSensorData(SensorData *data, TickType_t waitTicks = 0);
bool writeLatestSensorData(const SensorData &data, TickType_t waitTicks = portMAX_DELAY);
void notifyNewTemperature();
void notifyNewHumidity();

// --- LCD state helpers ---
DisplayState computeDisplayState(float temperature, float humidity);
void releaseStateSemaphore(DisplayState state);

#endif