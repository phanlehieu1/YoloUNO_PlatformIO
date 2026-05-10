#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__

#include <Arduino.h>
#include <Wire.h>
#include "DHT20.h"
#include "LiquidCrystal_I2C.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define I2C_SDA 11
#define I2C_SCL 12
#define LCD_I2C_ADDR 33
#define LCD_COLS 16
#define LCD_ROWS 2

struct SensorData
{
    float temperature;
    float humidity;
};

enum DisplayState
{
    STATE_NORMAL = 0,
    STATE_WARNING = 1,
    STATE_CRITICAL = 2
};

struct TempHumiMonitorContext
{
    QueueHandle_t sensorQueue;
    SemaphoreHandle_t dataMutex;
    SemaphoreHandle_t normalSemaphore;
    SemaphoreHandle_t warningSemaphore;
    SemaphoreHandle_t criticalSemaphore;
};

TempHumiMonitorContext *createTempHumiMonitorContext();
DisplayState computeDisplayState(float temperature, float humidity);
bool peekLatestSensorData(TempHumiMonitorContext *context, SensorData *data, TickType_t timeoutTicks);
void temp_humi_monitor(void *pvParameters);
void lcd_display_task(void *pvParameters);

#endif
