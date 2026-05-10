#ifndef __NEO_BLINKY__
#define __NEO_BLINKY__

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "global.h"

#define NEO_PIN 45
#define LED_COUNT 1

// Humidity thresholds for colour mapping
#define HUMIDITY_DRY_MAX     40.0f
#define HUMIDITY_COMFORT_MAX 70.0f

void neo_blinky(void *pvParameters);

#endif