#include "temp_humi_monitor.h"
#include <ArduinoJson.h>
#include "task_webserver.h"

// ============================================================
//  Sensor task — reads DHT20 every 5 seconds and publishes
//  the reading through the shared RTOS queue.
//  After writing, it:
//    1) Gives newTemperatureSemaphore  → wakes LED task  (Task 1)
//    2) Gives newHumiditySemaphore     → wakes NeoPixel  (Task 2)
//    3) Releases the correct LCD state → wakes LCD task   (Task 3)
// ============================================================

namespace {
constexpr TickType_t kSampleDelay = pdMS_TO_TICKS(5000);

const char *stateLabel(DisplayState state) {
  switch (state) {
    case STATE_CRITICAL: return "CRITICAL";
    case STATE_WARNING:  return "WARNING";
    default:             return "NORMAL";
  }
}
} // anonymous namespace

void temp_humi_monitor(void *pvParameters) {
  DHT20 dht20;
  Wire.begin(I2C_SDA, I2C_SCL);
  dht20.begin();

  while (1) {
    dht20.read();
    SensorData data;
    data.temperature = dht20.getTemperature();
    data.humidity    = dht20.getHumidity();

    if (isnan(data.temperature) || isnan(data.humidity)) {
      Serial.println("Failed to read from DHT20 sensor!");
      data.temperature = -1.0f;
      data.humidity    = -1.0f;
    }

    // Publish the latest reading into the single-slot queue (mutex-protected).
    if (writeLatestSensorData(data, pdMS_TO_TICKS(100))) {
      // Notify Task 1 (LED blink speed) and Task 2 (NeoPixel colour).
      notifyNewTemperature();
      notifyNewHumidity();
    }

    // Compute & release the correct LCD-state semaphore for Task 3.
    const DisplayState state = computeDisplayState(data.temperature, data.humidity);
    releaseStateSemaphore(state);

    Serial.printf("Humidity: %.1f%%  Temperature: %.1fC  State: %s\n",
                  data.humidity, data.temperature, stateLabel(state));

    // Send data over WebSocket to Dashboard
    StaticJsonDocument<128> doc;
    doc["temperature"] = data.temperature;
    doc["humidity"] = data.humidity;
    doc["lcd_state"] = stateLabel(state);
    
    String jsonString;
    serializeJson(doc, jsonString);
    Webserver_sendata(jsonString);

    vTaskDelay(kSampleDelay);
  }
}

// ============================================================
//  Task 3: LCD Display — shows temp/humidity with 3 states
//  NORMAL  → steady backlight, "[  NORMAL  ]"
//  WARNING → single flash,     "!  WARNING  !"
//  CRITICAL → triple flash,    "!! CRITICAL !!"
// ============================================================

void lcd_display_task(void *pvParameters) {
  LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Temp/Humi Task");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  while (1) {
    DisplayState state = STATE_NORMAL;

    // Condition semaphores are released by the sensor task.
    // Priority: CRITICAL > WARNING > NORMAL.
    if (xSemaphoreTake(criticalStateSemaphore(), pdMS_TO_TICKS(200)) == pdTRUE) {
      state = STATE_CRITICAL;
    } else if (xSemaphoreTake(warningStateSemaphore(), 0) == pdTRUE) {
      state = STATE_WARNING;
    } else if (xSemaphoreTake(normalStateSemaphore(), 0) == pdTRUE) {
      state = STATE_NORMAL;
    } else {
      continue;  // No state change → loop back
    }

    SensorData data = {0.0f, 0.0f};
    readLatestSensorData(&data, pdMS_TO_TICKS(200));

    char line[17];
    lcd.clear();
    lcd.setCursor(0, 0);
    snprintf(line, sizeof(line), "T:%4.1fC H:%3.0f%%", data.temperature, data.humidity);
    lcd.print(line);
    lcd.setCursor(0, 1);

    switch (state) {
      case STATE_CRITICAL:
        lcd.print("!! CRITICAL !! ");
        for (int i = 0; i < 3; ++i) {
          lcd.noBacklight();
          vTaskDelay(pdMS_TO_TICKS(120));
          lcd.backlight();
          vTaskDelay(pdMS_TO_TICKS(120));
        }
        break;
      case STATE_WARNING:
        lcd.print("!  WARNING  !  ");
        lcd.noBacklight();
        vTaskDelay(pdMS_TO_TICKS(250));
        lcd.backlight();
        break;
      case STATE_NORMAL:
      default:
        lcd.print("[  NORMAL   ] ");
        lcd.backlight();
        break;
    }
  }
}