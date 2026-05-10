#include "global.h"

QueueHandle_t xSensorQueue = NULL;
SemaphoreHandle_t xSensorQueueMutex = NULL;
SemaphoreHandle_t xSemaphoreNewTemp = NULL;
SemaphoreHandle_t xSemaphoreNewHumi = NULL;

String WIFI_SSID;
String WIFI_PASS;
String CORE_IOT_TOKEN;
String CORE_IOT_SERVER;
String CORE_IOT_PORT;

boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = NULL;

bool readLatestSensorData(SensorData *data, TickType_t timeoutTicks)
{
    if (data == NULL || xSensorQueue == NULL || xSensorQueueMutex == NULL) {
        return false;
    }

    if (xSemaphoreTake(xSensorQueueMutex, timeoutTicks) != pdTRUE) {
        return false;
    }

    bool hasData = xQueuePeek(xSensorQueue, data, 0) == pdTRUE;
    xSemaphoreGive(xSensorQueueMutex);
    return hasData;
}
