#include "global.h"

AppConfig &appConfig()
{
    static AppConfig config;
    return config;
}

SemaphoreHandle_t internetSemaphore()
{
    static SemaphoreHandle_t semaphore = xSemaphoreCreateBinary();
    return semaphore;
}
