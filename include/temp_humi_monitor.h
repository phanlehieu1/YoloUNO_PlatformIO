#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__

#include <Arduino.h>
#include <Wire.h>
#include "LiquidCrystal_I2C.h"
#include "DHT20.h"
#include "global.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define I2C_SDA 11
#define I2C_SCL 12
#define LCD_I2C_ADDR 33
#define LCD_COLS 16
#define LCD_ROWS 2

void temp_humi_monitor(void *pvParameters);
void lcd_display_task(void *pvParameters);

#endif