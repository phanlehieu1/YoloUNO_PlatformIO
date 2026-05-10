#include "coreiot.h"

// ============================================================
//  CoreIOT — MQTT client (uses accessor functions, no globals)
// ============================================================

namespace {
WiFiClient &espWifiClient() {
  static WiFiClient client;
  return client;
}

PubSubClient &mqttClient() {
  static PubSubClient client(espWifiClient());
  return client;
}
} // anonymous namespace

void reconnect() {
  while (!mqttClient().connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (mqttClient().connect(clientId.c_str())) {
      Serial.println("connected to CoreIOT Server!");
      mqttClient().subscribe("v1/devices/me/rpc/request/+");
      Serial.println("Subscribed to v1/devices/me/rpc/request/+");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient().state());
      Serial.println(" try again in 5 seconds");
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.print("Payload: ");
  Serial.println(message);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* method = doc["method"];
  if (strcmp(method, "setStateLED") == 0) {
    const char* params = doc["params"];
    if (strcmp(params, "ON") == 0) {
      Serial.println("Device turned ON.");
    } else {
      Serial.println("Device turned OFF.");
    }
  } else {
    Serial.print("Unknown method: ");
    Serial.println(method);
  }
}

void setup_coreiot() {
  // CoreIoT waits on this semaphore so MQTT starts only after Wi-Fi is ready.
  while (xSemaphoreTake(internetConnectedSemaphore(), portMAX_DELAY) != pdTRUE) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }

  const DeviceConfig config = getDeviceConfig();
  Serial.println(" Connected!");
  mqttClient().setServer(config.coreIotServer.c_str(), config.coreIotPort.toInt());
  mqttClient().setCallback(callback);
}

void coreiot_task(void *pvParameters) {
  setup_coreiot();

  while (1) {
    if (!mqttClient().connected()) {
      reconnect();
    }
    mqttClient().loop();

    SensorData data = {-1.0f, -1.0f};
    readLatestSensorData(&data, pdMS_TO_TICKS(50));
    String payload = "{\"temperature\":" + String(data.temperature) +
                     ",\"humidity\":" + String(data.humidity) + "}";

    mqttClient().publish("v1/devices/me/telemetry", payload.c_str());
    Serial.println("Published payload: " + payload);
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}