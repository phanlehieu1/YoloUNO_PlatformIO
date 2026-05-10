#include "global.h"

namespace {
QueueHandle_t &sensorQueueStorage()
{
  static QueueHandle_t handle = nullptr;
  return handle;
}

SemaphoreHandle_t &sensorMutexStorage()
{
  static SemaphoreHandle_t handle = nullptr;
  return handle;
}

SemaphoreHandle_t &temperatureSemaphoreStorage()
{
  static SemaphoreHandle_t handle = nullptr;
  return handle;
}

SemaphoreHandle_t &internetSemaphoreStorage()
{
  static SemaphoreHandle_t handle = nullptr;
  return handle;
}

SemaphoreHandle_t &configMutexStorage()
{
  static SemaphoreHandle_t handle = nullptr;
  return handle;
}

DeviceConfig &deviceConfigStorage()
{
  static DeviceConfig config;
  return config;
}
}

void initRtosResources()
{
  if (sensorQueueStorage() == nullptr) {
    sensorQueueStorage() = xQueueCreate(1, sizeof(SensorData));
  }
  if (sensorMutexStorage() == nullptr) {
    sensorMutexStorage() = xSemaphoreCreateMutex();
  }
  if (temperatureSemaphoreStorage() == nullptr) {
    temperatureSemaphoreStorage() = xSemaphoreCreateBinary();
  }
  if (internetSemaphoreStorage() == nullptr) {
    internetSemaphoreStorage() = xSemaphoreCreateBinary();
  }
  if (configMutexStorage() == nullptr) {
    configMutexStorage() = xSemaphoreCreateMutex();
  }
}

QueueHandle_t sensorQueue()
{
  return sensorQueueStorage();
}

SemaphoreHandle_t sensorDataMutex()
{
  return sensorMutexStorage();
}

SemaphoreHandle_t newTemperatureSemaphore()
{
  return temperatureSemaphoreStorage();
}

SemaphoreHandle_t internetConnectedSemaphore()
{
  return internetSemaphoreStorage();
}

SemaphoreHandle_t deviceConfigMutex()
{
  return configMutexStorage();
}

bool readLatestSensorData(SensorData *data, TickType_t waitTicks)
{
  if (data == nullptr || sensorQueueStorage() == nullptr || sensorMutexStorage() == nullptr) {
    return false;
  }

  if (xSemaphoreTake(sensorMutexStorage(), waitTicks) != pdTRUE) {
    return false;
  }

  const bool hasData = xQueuePeek(sensorQueueStorage(), data, 0) == pdTRUE;
  xSemaphoreGive(sensorMutexStorage());
  return hasData;
}

bool writeLatestSensorData(const SensorData &data, TickType_t waitTicks)
{
  if (sensorQueueStorage() == nullptr || sensorMutexStorage() == nullptr) {
    return false;
  }

  if (xSemaphoreTake(sensorMutexStorage(), waitTicks) != pdTRUE) {
    return false;
  }

  const bool wroteData = xQueueOverwrite(sensorQueueStorage(), &data) == pdTRUE;
  xSemaphoreGive(sensorMutexStorage());
  return wroteData;
}

void notifyNewTemperature()
{
  if (temperatureSemaphoreStorage() != nullptr) {
    xSemaphoreGive(temperatureSemaphoreStorage());
  }
}

DeviceConfig getDeviceConfig(TickType_t waitTicks)
{
  DeviceConfig config;
  if (configMutexStorage() != nullptr && xSemaphoreTake(configMutexStorage(), waitTicks) == pdTRUE) {
    config = deviceConfigStorage();
    xSemaphoreGive(configMutexStorage());
  }
  return config;
}

void setDeviceConfig(const DeviceConfig &config, TickType_t waitTicks)
{
  if (configMutexStorage() != nullptr && xSemaphoreTake(configMutexStorage(), waitTicks) == pdTRUE) {
    deviceConfigStorage() = config;
    xSemaphoreGive(configMutexStorage());
  }
}

bool hasWifiCredentials()
{
  const DeviceConfig config = getDeviceConfig();
  return !config.wifiSsid.isEmpty() || !config.wifiPass.isEmpty();
}
