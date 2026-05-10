#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

struct SensorData {
  float temperature;
  float humidity;
};

struct DeviceConfig {
  String wifiSsid;
  String wifiPass;
  String coreIotToken;
  String coreIotServer;
  String coreIotPort;
};

enum TemperatureCondition {
  TEMP_CONDITION_COLD,
  TEMP_CONDITION_NORMAL,
  TEMP_CONDITION_HOT,
  TEMP_CONDITION_SENSOR_ERROR
};

void initRtosResources();
QueueHandle_t sensorQueue();
SemaphoreHandle_t sensorDataMutex();
SemaphoreHandle_t newTemperatureSemaphore();
SemaphoreHandle_t internetConnectedSemaphore();
SemaphoreHandle_t deviceConfigMutex();

bool readLatestSensorData(SensorData *data, TickType_t waitTicks = 0);
bool writeLatestSensorData(const SensorData &data, TickType_t waitTicks = portMAX_DELAY);
void notifyNewTemperature();

DeviceConfig getDeviceConfig(TickType_t waitTicks = portMAX_DELAY);
void setDeviceConfig(const DeviceConfig &config, TickType_t waitTicks = portMAX_DELAY);
bool hasWifiCredentials();

#endif