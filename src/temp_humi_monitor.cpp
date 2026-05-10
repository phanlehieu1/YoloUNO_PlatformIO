#include "temp_humi_monitor.h"

namespace
{
    DHT20 dht20;
    LiquidCrystal_I2C lcd(33, 16, 2);
}

void temp_humi_monitor(void *pvParameters)
{
    Wire.begin(11, 12);
    Serial.begin(115200);
    dht20.begin();

    while (1) {
        dht20.read();

        SensorData data = {
            dht20.getTemperature(),
            dht20.getHumidity()
        };

        if (isnan(data.temperature) || isnan(data.humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            data.temperature = -1.0f;
            data.humidity = -1.0f;
        }

        if (xSemaphoreTake(xSensorQueueMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            xQueueOverwrite(xSensorQueue, &data);
            xSemaphoreGive(xSensorQueueMutex);

            // These semaphores notify condition-driven tasks that fresh sensor values are ready.
            xSemaphoreGive(xSemaphoreNewTemp);
            xSemaphoreGive(xSemaphoreNewHumi);
        }

        Serial.print("Humidity: ");
        Serial.print(data.humidity);
        Serial.print("%  Temperature: ");
        Serial.print(data.temperature);
        Serial.println("°C");

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
