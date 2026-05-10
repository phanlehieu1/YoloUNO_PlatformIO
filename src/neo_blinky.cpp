#include "neo_blinky.h"

// ============================================================
//  Task 2: NeoPixel LED colour changes based on humidity
//  - Dry    (<40%): amber (255, 180, 0)
//  - Comfort (40-70%): green (0, 255, 0)
//  - Humid  (>=70%): blue  (0, 0, 255)
//
//  The binary semaphore newHumiditySemaphore() wakes this task
//  whenever the sensor publishes a fresh reading.
// ============================================================

namespace {
/** Map a humidity value to a NeoPixel colour. */
uint32_t humidityColor(Adafruit_NeoPixel &strip, float humidity) {
  if (humidity < HUMIDITY_DRY_MAX)     return strip.Color(255, 180, 0);  // amber
  if (humidity < HUMIDITY_COMFORT_MAX) return strip.Color(0, 255, 0);    // green
  return strip.Color(0, 0, 255);                                        // blue
}

/** Return a human-readable label for the current humidity band. */
const char *humidityLabel(float humidity) {
  if (humidity < HUMIDITY_DRY_MAX)     return "DRY amber";
  if (humidity < HUMIDITY_COMFORT_MAX) return "COMFORT green";
  return "HUMID blue";
}
} // anonymous namespace

void neo_blinky(void *pvParameters) {
  Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
  strip.begin();
  strip.clear();
  strip.show();

  uint32_t currentColor = strip.Color(0, 0, 0);

  while (1) {
    // Block until the sensor task signals that a new humidity reading is ready.
    if (xSemaphoreTake(newHumiditySemaphore(), portMAX_DELAY) != pdTRUE) {
      continue;
    }

    SensorData data;
    if (!readLatestSensorData(&data, pdMS_TO_TICKS(100))) {
      continue;
    }

    uint32_t nextColor = humidityColor(strip, data.humidity);
    if (nextColor != currentColor) {
      currentColor = nextColor;
      strip.setPixelColor(0, currentColor);
      strip.show();
    }

    Serial.printf("[NEO] Humidity %.1f%% -> %s (<%.0f dry, %.0f-%.0f comfort, >=%.0f humid)\n",
                  data.humidity,
                  humidityLabel(data.humidity),
                  HUMIDITY_DRY_MAX,
                  HUMIDITY_DRY_MAX,
                  HUMIDITY_COMFORT_MAX,
                  HUMIDITY_COMFORT_MAX);
  }
}