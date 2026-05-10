#include "task_core_iot.h"
#include <WiFi.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>
#include "task_handler.h"

#if !defined(DEVICE_ROLE_ACTUATOR)
#include "tinyml.h"
#endif

namespace {
constexpr uint32_t kTelemetryIntervalMs = 10000;
constexpr uint16_t kThingsBoardBufferSize = 1024;

WiFiClient &coreiotWifiClient() {
    static WiFiClient client;
    return client;
}

Arduino_MQTT_Client &coreiotMqttClient() {
    static Arduino_MQTT_Client client(coreiotWifiClient());
    return client;
}

ThingsBoard &coreiotClient() {
    static ThingsBoard client(coreiotMqttClient(), kThingsBoardBufferSize);
    return client;
}

bool &rpcSubscribed() {
    static bool subscribed = false;
    return subscribed;
}

void waitForInternetConnection() {
    Serial.println("[CoreIOT] Waiting for internet connection semaphore...");
    while (xSemaphoreTake(internetConnectedSemaphore(), portMAX_DELAY) != pdTRUE) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    xSemaphoreGive(internetConnectedSemaphore());
    Serial.printf("[CoreIOT] Internet connected. Local IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.println("[CoreIOT] Starting task loop...");
}

bool connectCoreIot(ThingsBoard &tb) {
    DeviceConfig config = getDeviceConfig();
    String server = config.coreIotServer;
    String token = config.coreIotToken;

#if defined(DEVICE_ROLE_ACTUATOR)
    if (token.isEmpty()) {
        token = String(DEVICE_CORE_IOT_TOKEN);
    }
#endif

    server.trim();
    token.trim();
    int port = config.coreIotPort.toInt();
    if (port <= 0) {
        port = 1883;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.printf("[CoreIOT] WiFi not connected yet. Status=%d\n", WiFi.status());
        rpcSubscribed() = false;
        return false;
    }

    if (token.isEmpty() || server.isEmpty()) {
        Serial.println("[CoreIOT] Missing token/server config.");
        return false;
    }

    if (tb.connected()) {
        return true;
    }

    String clientId = "YoloUNO-" + WiFi.macAddress();
    clientId.replace(":", "");

    Serial.printf("[CoreIOT] Connecting to %s:%d with clientId=%s\n", server.c_str(), port, clientId.c_str());
    if (!tb.connect(server.c_str(), token.c_str(), port, clientId.c_str())) {
        Serial.println("[CoreIOT] Connection failed.");
        rpcSubscribed() = false;
        return false;
    }

    Serial.println("[CoreIOT] Connected successfully!");
    rpcSubscribed() = false;
    tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
    tb.sendAttributeData("localIp", WiFi.localIP().toString().c_str());
#if defined(DEVICE_ROLE_ACTUATOR)
    tb.sendAttributeData("deviceRole", "actuator");
#else
    tb.sendAttributeData("deviceRole", "sensor");
#endif
    return true;
}

#if defined(DEVICE_ROLE_ACTUATOR)
RPC_Callback powerCallback("POWER", handlePowerRpc);
RPC_Callback powerLowerCallback("power", handlePowerRpc);
RPC_Callback setValueCallback("setValue", handlePowerRpc);

bool ensureRpcSubscription(ThingsBoard &tb) {
    if (rpcSubscribed()) {
        return true;
    }

    if (!tb.RPC_Subscribe(powerCallback)) {
        Serial.println("[CoreIOT] RPC subscribe failed for POWER.");
        return false;
    }
    if (!tb.RPC_Subscribe(powerLowerCallback)) {
        Serial.println("[CoreIOT] RPC subscribe failed for power.");
        return false;
    }
    if (!tb.RPC_Subscribe(setValueCallback)) {
        Serial.println("[CoreIOT] RPC subscribe failed for setValue.");
        return false;
    }

    rpcSubscribed() = true;
    Serial.println("[CoreIOT] RPC subscribed for actuator role.");
    return true;
}
#else
void publishSensorTelemetry(ThingsBoard &tb) {
    static uint32_t lastSendMs = 0;
    if (millis() - lastSendMs < kTelemetryIntervalMs) {
        return;
    }
    lastSendMs = millis();

    SensorData data;
    if (!readLatestSensorData(&data, 0)) {
        Serial.println("[CoreIOT] Sensor data not ready yet.");
        return;
    }

    TinyMLState mlState = getTinyMLState();
    tb.sendTelemetryData("temperature", data.temperature);
    tb.sendTelemetryData("humidity", data.humidity);
    tb.sendTelemetryData("anomaly_score", mlState.lastScore);
    tb.sendTelemetryData("is_anomaly", mlState.isAnomaly);
    tb.sendTelemetryData("lat", 10.772175109674038);
    tb.sendTelemetryData("long", 106.65789107082472);

    Serial.printf("[CoreIOT] Published -> T:%.1f, H:%.1f, Score:%.4f, Anomaly:%s\n",
                  data.temperature, data.humidity, mlState.lastScore,
                  mlState.isAnomaly ? "YES" : "NO");
}
#endif
} // namespace

void coreiot_task(void *pvParameters) {
    waitForInternetConnection();
    ThingsBoard &tb = coreiotClient();

    while (1) {
        if (!connectCoreIot(tb)) {
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

#if defined(DEVICE_ROLE_ACTUATOR)
        ensureRpcSubscription(tb);
#else
        publishSensorTelemetry(tb);
#endif

        tb.loop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
