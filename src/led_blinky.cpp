#include "led_blinky.h"

// ============================================================
//  Task 1: LED blinks at different speeds based on temperature
//  - Cold  (<24°C): slow blink (1000ms)
//  - Normal (24-30°C): medium blink (500ms)
//  - Hot   (>=30°C): fast blink (125ms)
//  - Sensor error: very slow blink (2000ms)
//
//  The binary semaphore newTemperatureSemaphore() wakes this
//  task whenever the sensor publishes a fresh reading.
// ============================================================

namespace {
constexpr float TEMP_COLD_MAX_C = 24.0f;
constexpr float TEMP_HOT_MIN_C  = 30.0f;

/** Classify current temperature into one of four conditions. */
TemperatureCondition getTemperatureCondition(float temperature) {
  if (temperature < 0.0f || isnan(temperature)) return TEMP_CONDITION_SENSOR_ERROR;
  if (temperature < TEMP_COLD_MAX_C)            return TEMP_CONDITION_COLD;
  if (temperature < TEMP_HOT_MIN_C)             return TEMP_CONDITION_NORMAL;
  return TEMP_CONDITION_HOT;
}

/** Return the on/off delay for the LED based on the temperature condition. */
TickType_t blinkDelayForCondition(TemperatureCondition condition) {
  switch (condition) {
    case TEMP_CONDITION_COLD:         return pdMS_TO_TICKS(1000);
    case TEMP_CONDITION_NORMAL:       return pdMS_TO_TICKS(500);
    case TEMP_CONDITION_HOT:          return pdMS_TO_TICKS(125);
    case TEMP_CONDITION_SENSOR_ERROR:
    default:                          return pdMS_TO_TICKS(2000);
  }
}
} // anonymous namespace

void led_blinky(void *pvParameters) {
  pinMode(LED_GPIO, OUTPUT);
  TickType_t blinkDelay = pdMS_TO_TICKS(1000);  // default until first reading

  while (1) {
    // Wait up to 100ms for a new temperature notification from the sensor task.
    if (xSemaphoreTake(newTemperatureSemaphore(), pdMS_TO_TICKS(100)) == pdTRUE) {
      SensorData data;
      // Read the latest sensor data from the shared queue (mutex-protected peek).
      if (readLatestSensorData(&data, pdMS_TO_TICKS(50))) {
        blinkDelay = blinkDelayForCondition(getTemperatureCondition(data.temperature));
      }
    }

    digitalWrite(LED_GPIO, HIGH);
    vTaskDelay(blinkDelay);
    digitalWrite(LED_GPIO, LOW);
    vTaskDelay(blinkDelay);
  }
}