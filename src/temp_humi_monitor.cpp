#include "temp_humi_monitor.h"

namespace {
DHT20 &dht20Sensor()
{
    static DHT20 dht20;
    return dht20;
}

LiquidCrystal_I2C &lcdDisplay()
{
    static LiquidCrystal_I2C lcd(33, 16, 2);
    return lcd;
}
}

void temp_humi_monitor(void *pvParameters)
{
    Wire.begin(11, 12);
    dht20Sensor().begin();
    lcdDisplay();

    while (1) {
        dht20Sensor().read();

        SensorData data;
        data.temperature = dht20Sensor().getTemperature();
        data.humidity = dht20Sensor().getHumidity();

        if (isnan(data.temperature) || isnan(data.humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            data.temperature = -1.0f;
            data.humidity = -1.0f;
        }

        // The sensor queue carries the newest reading; the semaphore tells LED logic to re-check its condition.
        if (writeLatestSensorData(data, pdMS_TO_TICKS(100))) {
            notifyNewTemperature();
        }

        Serial.print("Humidity: ");
        Serial.print(data.humidity);
        Serial.print("%  Temperature: ");
        Serial.print(data.temperature);
        Serial.println("°C");

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
