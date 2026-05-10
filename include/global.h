#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

struct AppConfig
{
    String wifiSsid;
    String wifiPass;
    String coreIotToken;
    String coreIotServer;
    String coreIotPort;
};

AppConfig &appConfig();
SemaphoreHandle_t internetSemaphore();

#endif
