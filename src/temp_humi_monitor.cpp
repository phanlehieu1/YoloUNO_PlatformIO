#include "temp_humi_monitor.h"

namespace
{
    constexpr TickType_t kSampleDelay = pdMS_TO_TICKS(5000);

    const char *stateLabel(DisplayState state)
    {
        switch (state)
        {
        case STATE_CRITICAL:
            return "CRITICAL";
        case STATE_WARNING:
            return "WARNING";
        case STATE_NORMAL:
        default:
            return "NORMAL";
        }
    }

    void releaseStateSemaphore(TempHumiMonitorContext *context, DisplayState state)
    {
        // Only the semaphore for the active condition is released, so the LCD task handles one clear state change.
        while (xSemaphoreTake(context->normalSemaphore, 0) == pdTRUE) {}
        while (xSemaphoreTake(context->warningSemaphore, 0) == pdTRUE) {}
        while (xSemaphoreTake(context->criticalSemaphore, 0) == pdTRUE) {}

        SemaphoreHandle_t target = context->normalSemaphore;
        if (state == STATE_WARNING)
        {
            target = context->warningSemaphore;
        }
        else if (state == STATE_CRITICAL)
        {
            target = context->criticalSemaphore;
        }
        xSemaphoreGive(target);
    }
}

TempHumiMonitorContext *createTempHumiMonitorContext()
{
    TempHumiMonitorContext *context = new TempHumiMonitorContext{
        xQueueCreate(1, sizeof(SensorData)),
        xSemaphoreCreateMutex(),
        xSemaphoreCreateBinary(),
        xSemaphoreCreateBinary(),
        xSemaphoreCreateBinary()};

    if (context->sensorQueue == NULL || context->dataMutex == NULL || context->normalSemaphore == NULL ||
        context->warningSemaphore == NULL || context->criticalSemaphore == NULL)
    {
        Serial.println("Failed to create temperature/humidity RTOS resources");
    }
    return context;
}

DisplayState computeDisplayState(float temperature, float humidity)
{
    if (temperature >= 35.0f || humidity >= 80.0f)
    {
        return STATE_CRITICAL;
    }
    if (temperature >= 30.0f || humidity >= 65.0f)
    {
        return STATE_WARNING;
    }
    return STATE_NORMAL;
}

bool peekLatestSensorData(TempHumiMonitorContext *context, SensorData *data, TickType_t timeoutTicks)
{
    if (context == NULL || data == NULL || context->dataMutex == NULL || context->sensorQueue == NULL)
    {
        return false;
    }

    if (xSemaphoreTake(context->dataMutex, timeoutTicks) != pdTRUE)
    {
        return false;
    }
    const bool hasData = xQueuePeek(context->sensorQueue, data, 0) == pdTRUE;
    xSemaphoreGive(context->dataMutex);
    return hasData;
}

void temp_humi_monitor(void *pvParameters)
{
    TempHumiMonitorContext *context = static_cast<TempHumiMonitorContext *>(pvParameters);
    if (context == NULL)
    {
        vTaskDelete(NULL);
    }

    DHT20 dht20;
    Wire.begin(I2C_SDA, I2C_SCL);
    dht20.begin();

    while (1)
    {
        dht20.read();
        SensorData data = {dht20.getTemperature(), dht20.getHumidity()};

        if (isnan(data.temperature) || isnan(data.humidity))
        {
            Serial.println("Failed to read from DHT20 sensor");
            vTaskDelay(kSampleDelay);
            continue;
        }

        // The mutex protects the single-slot queue while the latest sensor sample replaces the old one.
        if (xSemaphoreTake(context->dataMutex, pdMS_TO_TICKS(200)) == pdTRUE)
        {
            xQueueOverwrite(context->sensorQueue, &data);
            xSemaphoreGive(context->dataMutex);
        }

        const DisplayState state = computeDisplayState(data.temperature, data.humidity);
        releaseStateSemaphore(context, state);

        Serial.printf("Humidity: %.1f%%  Temperature: %.1fC  State: %s\n", data.humidity, data.temperature, stateLabel(state));
        vTaskDelay(kSampleDelay);
    }
}

void lcd_display_task(void *pvParameters)
{
    TempHumiMonitorContext *context = static_cast<TempHumiMonitorContext *>(pvParameters);
    if (context == NULL)
    {
        vTaskDelete(NULL);
    }

    LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Temp/Humi Task");
    lcd.setCursor(0, 1);
    lcd.print("Starting...");

    while (1)
    {
        DisplayState state = STATE_NORMAL;

        // Condition semaphores are released by the sensor task when readings cross a display threshold.
        if (xSemaphoreTake(context->criticalSemaphore, pdMS_TO_TICKS(200)) == pdTRUE)
        {
            state = STATE_CRITICAL;
        }
        else if (xSemaphoreTake(context->warningSemaphore, 0) == pdTRUE)
        {
            state = STATE_WARNING;
        }
        else if (xSemaphoreTake(context->normalSemaphore, 0) == pdTRUE)
        {
            state = STATE_NORMAL;
        }
        else
        {
            continue;
        }

        SensorData data = {0.0f, 0.0f};
        peekLatestSensorData(context, &data, pdMS_TO_TICKS(200));

        char line[17];
        lcd.clear();
        lcd.setCursor(0, 0);
        snprintf(line, sizeof(line), "T:%4.1fC H:%3.0f%%", data.temperature, data.humidity);
        lcd.print(line);
        lcd.setCursor(0, 1);

        switch (state)
        {
        case STATE_CRITICAL:
            lcd.print("!! CRITICAL !! ");
            for (int i = 0; i < 3; ++i)
            {
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
