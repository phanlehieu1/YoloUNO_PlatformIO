#ifndef __TASK_CORE_IOT_H__
#define __TASK_CORE_IOT_H__

#include <Arduino.h>
#include "global.h"

// FreeRTOS task to handle CoreIOT (ThingsBoard) data publishing
void coreiot_task(void *pvParameters);

#endif