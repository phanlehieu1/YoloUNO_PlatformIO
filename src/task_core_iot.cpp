#include "task_core_iot.h"
#include <WiFi.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>

#include "tinyml.h"

void coreiot_task(void *pvParameters) {
    // 1. Wait until Wi-Fi connects (Signaled by task_wifi)
    Serial.println("[CoreIOT] Waiting for internet connection semaphore...");
    while (xSemaphoreTake(internetConnectedSemaphore(), portMAX_DELAY) != pdTRUE) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    // Give it back so other tasks aren't blocked if they check it.
    xSemaphoreGive(internetConnectedSemaphore());

    Serial.println("[CoreIOT] Internet connected. Starting task loop...");

    WiFiClient wifiClient;
    Arduino_MQTT_Client mqttClient(wifiClient);
    ThingsBoard tb(mqttClient, 1024);

    uint32_t lastSendMs = 0;
    const uint32_t intervalMs = 10000;

    while (1) {
        // Always get fresh config in case it was updated via web portal
        DeviceConfig config = getDeviceConfig();
        String server = config.coreIotServer; server.trim();
        String token = config.coreIotToken; token.trim();
        int port = config.coreIotPort.toInt();
        if (port <= 0) port = 1883;

        if (token.isEmpty() || server.isEmpty()) {
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // 3. Handle Connection
        if (!tb.connected()) {
            // Unique Client ID to avoid "duplicate connection" kicks
            String clientId = "YoloUNO-" + WiFi.macAddress();
            clientId.replace(":", "");
            
            Serial.printf("[CoreIOT] Connecting to %s:%d with token %s...\n", server.c_str(), port, token.c_str());
            if (!tb.connect(server.c_str(), token.c_str(), port, clientId.c_str())) {
                Serial.println("[CoreIOT] Connection failed. Retrying in 5s...");
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
            Serial.println("[CoreIOT] Connected successfully!");
            
            // Send initial attributes
            tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
            tb.sendAttributeData("localIp", WiFi.localIP().toString().c_str());
        }

        // 4. Periodic Data Publishing
        if (millis() - lastSendMs >= intervalMs) {
            lastSendMs = millis();
            
            SensorData data;
            // Accessor helper from global.h
            if (readLatestSensorData(&data, 0)) {
                // Get latest TinyML results
                TinyMLState mlState = getTinyMLState();

                // Core Telemetry
                tb.sendTelemetryData("temperature", data.temperature);
                tb.sendTelemetryData("humidity", data.humidity);
                
                // Anomaly Telemetry
                tb.sendTelemetryData("anomaly_score", mlState.lastScore);
                tb.sendTelemetryData("is_anomaly", mlState.isAnomaly);
                
                // Location Telemetry (User's location)
                tb.sendTelemetryData("lat", 10.772175109674038);
                tb.sendTelemetryData("long", 106.65789107082472);
                
                Serial.printf("[CoreIOT] Published -> T:%.1f, H:%.1f, Score:%.4f, Anomaly:%s\n", 
                              data.temperature, data.humidity, mlState.lastScore, 
                              mlState.isAnomaly ? "YES" : "NO");
            } else {
                Serial.println("[CoreIOT] Sensor data not ready yet.");
            }
        }

        // 5. Keep alive and handle callbacks (if any)
        tb.loop();
        
        // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}