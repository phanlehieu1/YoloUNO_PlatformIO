#include "global.h"

// ============================================================
//  Private storage — all handles live inside accessor functions
//  so there are zero file-scope globals.
// ============================================================

namespace {

// --- Sensor Queue ---
QueueHandle_t &sensorQueueStorage() {
  static QueueHandle_t handle = nullptr;
  return handle;
}

// --- Sensor Mutex ---
SemaphoreHandle_t &sensorMutexStorage() {
  static SemaphoreHandle_t handle = nullptr;
  return handle;
}

// --- Temperature notification (binary semaphore) ---
SemaphoreHandle_t &temperatureSemaphoreStorage() {
  static SemaphoreHandle_t handle = nullptr;
  return handle;
}

// --- Humidity notification (binary semaphore) ---
SemaphoreHandle_t &humiditySemaphoreStorage() {
  static SemaphoreHandle_t handle = nullptr;
  return handle;
}

// --- Internet connection semaphore ---
SemaphoreHandle_t &internetSemaphoreStorage() {
  static SemaphoreHandle_t handle = nullptr;
  return handle;
}

// --- Config Mutex ---
SemaphoreHandle_t &configMutexStorage() {
  static SemaphoreHandle_t handle = nullptr;
  return handle;
}

// --- Config data ---
DeviceConfig &deviceConfigStorage() {
  static DeviceConfig config;
  return config;
}

// --- LCD state semaphores ---
SemaphoreHandle_t &normalSemStorage() {
  static SemaphoreHandle_t handle = nullptr;
  return handle;
}
SemaphoreHandle_t &warningSemStorage() {
  static SemaphoreHandle_t handle = nullptr;
  return handle;
}
SemaphoreHandle_t &criticalSemStorage() {
  static SemaphoreHandle_t handle = nullptr;
  return handle;
}

} // anonymous namespace

// ============================================================
//  initRtosResources — must be called once in setup()
// ============================================================
void initRtosResources() {
  if (!sensorQueueStorage())
    sensorQueueStorage() = xQueueCreate(1, sizeof(SensorData));
  if (!sensorMutexStorage())
    sensorMutexStorage() = xSemaphoreCreateMutex();
  if (!temperatureSemaphoreStorage())
    temperatureSemaphoreStorage() = xSemaphoreCreateBinary();
  if (!humiditySemaphoreStorage())
    humiditySemaphoreStorage() = xSemaphoreCreateBinary();
  if (!internetSemaphoreStorage())
    internetSemaphoreStorage() = xSemaphoreCreateBinary();
  if (!configMutexStorage())
    configMutexStorage() = xSemaphoreCreateMutex();
  if (!normalSemStorage())
    normalSemStorage() = xSemaphoreCreateBinary();
  if (!warningSemStorage())
    warningSemStorage() = xSemaphoreCreateBinary();
  if (!criticalSemStorage())
    criticalSemStorage() = xSemaphoreCreateBinary();
}

// ============================================================
//  Public accessor functions
// ============================================================

QueueHandle_t sensorQueue()                 { return sensorQueueStorage(); }
SemaphoreHandle_t sensorDataMutex()         { return sensorMutexStorage(); }
SemaphoreHandle_t newTemperatureSemaphore()  { return temperatureSemaphoreStorage(); }
SemaphoreHandle_t newHumiditySemaphore()     { return humiditySemaphoreStorage(); }
SemaphoreHandle_t internetConnectedSemaphore() { return internetSemaphoreStorage(); }
SemaphoreHandle_t deviceConfigMutex()       { return configMutexStorage(); }
SemaphoreHandle_t normalStateSemaphore()    { return normalSemStorage(); }
SemaphoreHandle_t warningStateSemaphore()   { return warningSemStorage(); }
SemaphoreHandle_t criticalStateSemaphore()  { return criticalSemStorage(); }

// ============================================================
//  Sensor data helpers (mutex + single-slot queue)
// ============================================================

bool readLatestSensorData(SensorData *data, TickType_t waitTicks) {
  if (!data || !sensorQueueStorage() || !sensorMutexStorage()) return false;
  if (xSemaphoreTake(sensorMutexStorage(), waitTicks) != pdTRUE) return false;
  const bool ok = xQueuePeek(sensorQueueStorage(), data, 0) == pdTRUE;
  xSemaphoreGive(sensorMutexStorage());
  return ok;
}

bool writeLatestSensorData(const SensorData &data, TickType_t waitTicks) {
  if (!sensorQueueStorage() || !sensorMutexStorage()) return false;
  if (xSemaphoreTake(sensorMutexStorage(), waitTicks) != pdTRUE) return false;
  const bool ok = xQueueOverwrite(sensorQueueStorage(), &data) == pdTRUE;
  xSemaphoreGive(sensorMutexStorage());
  return ok;
}

/** Wake the LED-blink task so it can re-evaluate the temperature condition. */
void notifyNewTemperature() {
  if (temperatureSemaphoreStorage())
    xSemaphoreGive(temperatureSemaphoreStorage());
}

/** Wake the NeoPixel task so it can re-evaluate the humidity condition. */
void notifyNewHumidity() {
  if (humiditySemaphoreStorage())
    xSemaphoreGive(humiditySemaphoreStorage());
}

// ============================================================
//  Device config helpers (mutex-protected)
// ============================================================

DeviceConfig getDeviceConfig(TickType_t waitTicks) {
  DeviceConfig config;
  if (configMutexStorage() && xSemaphoreTake(configMutexStorage(), waitTicks) == pdTRUE) {
    config = deviceConfigStorage();
    xSemaphoreGive(configMutexStorage());
  }
  return config;
}

void setDeviceConfig(const DeviceConfig &config, TickType_t waitTicks) {
  if (configMutexStorage() && xSemaphoreTake(configMutexStorage(), waitTicks) == pdTRUE) {
    deviceConfigStorage() = config;
    xSemaphoreGive(configMutexStorage());
  }
}

bool hasWifiCredentials() {
  const DeviceConfig config = getDeviceConfig();
  return !config.wifiSsid.isEmpty() || !config.wifiPass.isEmpty();
}

String getFirmwareVersion() {
  return "fw_v2"; // Task 1 Web OTA Version
}

// ============================================================
//  LCD display state helpers (Task 3)
// ============================================================

/** Map temperature + humidity to one of three display states. */
DisplayState computeDisplayState(float temperature, float humidity) {
  if (temperature >= 35.0f || humidity >= 80.0f) return STATE_CRITICAL;
  if (temperature >= 30.0f || humidity >= 65.0f) return STATE_WARNING;
  return STATE_NORMAL;
}

/**
 * Release only the semaphore matching the current state.
 * Clears the other two first so the LCD task always sees a single, clean state change.
 */
void releaseStateSemaphore(DisplayState state) {
  // Drain all three semaphores first
  while (xSemaphoreTake(normalSemStorage(), 0) == pdTRUE) {}
  while (xSemaphoreTake(warningSemStorage(), 0) == pdTRUE) {}
  while (xSemaphoreTake(criticalSemStorage(), 0) == pdTRUE) {}

  switch (state) {
    case STATE_CRITICAL: xSemaphoreGive(criticalSemStorage()); break;
    case STATE_WARNING:  xSemaphoreGive(warningSemStorage());  break;
    default:             xSemaphoreGive(normalSemStorage());   break;
  }
}