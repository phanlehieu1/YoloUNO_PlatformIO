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

extern QueueHandle_t xSensorQueue;
extern SemaphoreHandle_t xSensorQueueMutex;
extern SemaphoreHandle_t xSemaphoreNewTemp;
extern SemaphoreHandle_t xSemaphoreNewHumi;

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

bool readLatestSensorData(SensorData *data, TickType_t timeoutTicks);

#endif