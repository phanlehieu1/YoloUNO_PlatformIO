#include "led_blinky.h"

namespace {
constexpr float TEMP_COLD_MAX_C = 24.0f;
constexpr float TEMP_HOT_MIN_C = 30.0f;
constexpr TickType_t TEMP_EVENT_WAIT = pdMS_TO_TICKS(100);

TemperatureCondition getTemperatureCondition(float temperature)
{
  if (temperature < 0.0f || isnan(temperature)) {
    return TEMP_CONDITION_SENSOR_ERROR;
  }
  if (temperature < TEMP_COLD_MAX_C) {
    return TEMP_CONDITION_COLD;
  }
  if (temperature < TEMP_HOT_MIN_C) {
    return TEMP_CONDITION_NORMAL;
  }
  return TEMP_CONDITION_HOT;
}

TickType_t blinkDelayForCondition(TemperatureCondition condition)
{
  switch (condition) {
    case TEMP_CONDITION_COLD:
      return pdMS_TO_TICKS(1000);
    case TEMP_CONDITION_NORMAL:
      return pdMS_TO_TICKS(500);
    case TEMP_CONDITION_HOT:
      return pdMS_TO_TICKS(125);
    case TEMP_CONDITION_SENSOR_ERROR:
    default:
      return pdMS_TO_TICKS(2000);
  }
}
}

void led_blinky(void *pvParameters)
{
  pinMode(LED_GPIO, OUTPUT);

  TickType_t blinkDelay = pdMS_TO_TICKS(1000);

  while (1) {
    // The semaphore wakes this task only when the sensor task has published a fresh temperature sample.
    if (xSemaphoreTake(newTemperatureSemaphore(), TEMP_EVENT_WAIT) == pdTRUE) {
      SensorData data;
      // The mutex keeps the one-slot queue consistent while the latest sample is copied without consuming it.
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
