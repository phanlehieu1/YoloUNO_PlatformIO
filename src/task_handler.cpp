#include <task_handler.h>
#include <task_webserver.h>
#include <Adafruit_NeoPixel.h>

namespace {
constexpr int kDefaultActuatorGpio = 48;
constexpr uint8_t kActuatorPixelCount = 1;
constexpr uint8_t kActuatorBrightness = 50;

Adafruit_NeoPixel &actuatorPixels() {
    static Adafruit_NeoPixel pixels(kActuatorPixelCount, kDefaultActuatorGpio, NEO_GRB + NEO_KHZ800);
    return pixels;
}

void ensureActuatorPixelsReady() {
    static bool initialized = false;
    if (initialized) {
        return;
    }

    actuatorPixels().begin();
    actuatorPixels().setBrightness(kActuatorBrightness);
    actuatorPixels().setPixelColor(0, actuatorPixels().Color(0, 0, 0));
    actuatorPixels().show();
    initialized = true;
}

void setActuatorPixelColor(uint8_t red, uint8_t green, uint8_t blue) {
    ensureActuatorPixelsReady();
    actuatorPixels().setPixelColor(0, actuatorPixels().Color(red, green, blue));
    actuatorPixels().show();
}

String normalizeStatus(String status) {
    status.trim();
    status.replace("\"", "");

    if (status.equalsIgnoreCase("ON") || status.equalsIgnoreCase("TRUE") || status == "1") {
        return "ON";
    }
    if (status.equalsIgnoreCase("OFF") || status.equalsIgnoreCase("FALSE") || status == "0") {
        return "OFF";
    }
    return status;
}

String rpcParamToStatus(const JsonVariantConst &data) {
    if (data.is<bool>()) {
        return data.as<bool>() ? "ON" : "OFF";
    }
    if (data.is<int>()) {
        return data.as<int>() != 0 ? "ON" : "OFF";
    }
    if (data.is<const char *>()) {
        return normalizeStatus(String(data.as<const char *>()));
    }
    if (data.is<String>()) {
        return normalizeStatus(data.as<String>());
    }
    if (data.is<JsonObjectConst>()) {
        JsonObjectConst obj = data.as<JsonObjectConst>();
        if (obj.containsKey("status")) {
            if (obj["status"].is<bool>()) {
                return obj["status"].as<bool>() ? "ON" : "OFF";
            }
            if (obj["status"].is<int>()) {
                return obj["status"].as<int>() != 0 ? "ON" : "OFF";
            }
            if (obj["status"].is<const char *>()) {
                return normalizeStatus(String(obj["status"].as<const char *>()));
            }
            return normalizeStatus(obj["status"].as<String>());
        }
        if (obj.containsKey("params")) {
            if (obj["params"].is<bool>()) {
                return obj["params"].as<bool>() ? "ON" : "OFF";
            }
            if (obj["params"].is<int>()) {
                return obj["params"].as<int>() != 0 ? "ON" : "OFF";
            }
            if (obj["params"].is<const char *>()) {
                return normalizeStatus(String(obj["params"].as<const char *>()));
            }
            return normalizeStatus(obj["params"].as<String>());
        }
        if (obj.containsKey("value")) {
            if (obj["value"].is<bool>()) {
                return obj["value"].as<bool>() ? "ON" : "OFF";
            }
            if (obj["value"].is<int>()) {
                return obj["value"].as<int>() != 0 ? "ON" : "OFF";
            }
            if (obj["value"].is<const char *>()) {
                return normalizeStatus(String(obj["value"].as<const char *>()));
            }
            return normalizeStatus(obj["value"].as<String>());
        }
    }
    return String();
}

int rpcParamToGpio(const JsonVariantConst &data) {
    if (data.is<JsonObjectConst>()) {
        JsonObjectConst obj = data.as<JsonObjectConst>();
        if (obj.containsKey("gpio")) {
            return obj["gpio"].as<int>();
        }
    }
    return kDefaultActuatorGpio;
}
} // namespace

bool applyDevicePowerCommand(int gpio, const String &status)
{
    Serial.printf("⚙️ Điều khiển GPIO %d → %s\n", gpio, status.c_str());

    if (gpio != kDefaultActuatorGpio) {
        Serial.printf("⚠️ GPIO %d không hỗ trợ trên actuator NeoPixel hiện tại\n", gpio);
        return false;
    }

    if (status.equalsIgnoreCase("ON"))
    {
        setActuatorPixelColor(255, 0, 0);
        Serial.printf("🔆 GPIO %d ON\n", gpio);
        return true;
    }

    if (status.equalsIgnoreCase("OFF"))
    {
        setActuatorPixelColor(0, 0, 0);
        Serial.printf("💤 GPIO %d OFF\n", gpio);
        return true;
    }

    Serial.printf("⚠️ Trạng thái không hợp lệ cho GPIO %d: %s\n", gpio, status.c_str());
    return false;
}

RPC_Response handlePowerRpc(const JsonVariantConst &data)
{
    String rawPayload;
    serializeJson(data, rawPayload);
    Serial.println("[CoreIOT] RPC callback triggered.");
    Serial.println("[CoreIOT] RPC params: " + rawPayload);

    const int gpio = rpcParamToGpio(data);
    const String status = rpcParamToStatus(data);

    if (status.isEmpty()) {
        Serial.println("⚠️ RPC thiếu params/status");
        return RPC_Response("success", false);
    }

    const bool ok = applyDevicePowerCommand(gpio, status);
    Serial.printf("[CoreIOT] RPC result -> gpio=%d, status=%s, ok=%s\n",
                  gpio, status.c_str(), ok ? "true" : "false");
    return RPC_Response("success", ok);
}

void handleWebSocketMessage(String message)
{
    Serial.println(message);
    StaticJsonDocument<512> doc;

    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        Serial.println("❌ Lỗi parse JSON!");
        return;
    }
    JsonObject value = doc["value"];
    if (doc["page"] == "device")
    {
        if (!value.containsKey("gpio") || !value.containsKey("status"))
        {
            Serial.println("⚠️ JSON thiếu thông tin gpio hoặc status");
            return;
        }

        int gpio = value["gpio"];
        String status = value["status"].as<String>();
        applyDevicePowerCommand(gpio, status);
    }
    else if (doc["page"] == "setting")
    {
        String wifi_ssid = doc["value"]["ssid"].as<String>();
        String wifi_pass = doc["value"]["password"].as<String>();
        String core_iot_token = doc["value"]["token"].as<String>();
        String core_iot_server = doc["value"]["server"].as<String>();
        String core_iot_port = doc["value"]["port"].as<String>();

        Serial.println("📥 Nhận cấu hình từ WebSocket:");
        Serial.println("SSID: " + wifi_ssid);
        Serial.println("PASS: " + wifi_pass);
        Serial.println("TOKEN: " + core_iot_token);
        Serial.println("SERVER: " + core_iot_server);
        Serial.println("PORT: " + core_iot_port);

        Save_info_File(wifi_ssid, wifi_pass, core_iot_token, core_iot_server, core_iot_port);

        String msg = "{\"status\":\"ok\",\"page\":\"setting_saved\"}";
        Webserver_sendata(msg);
    }
    else if (doc["page"] == "retrain")
    {
        String action = value["action"].as<String>();
        String dataset = value["dataset"].as<String>();

        Serial.println("[TinyML] Retrain request received from dashboard.");
        Serial.println("[TinyML] Action: " + action);
        Serial.println("[TinyML] Dataset: " + dataset);

        StaticJsonDocument<192> response;
        response["page"] = "retrain_status";

        if (action != "start") {
            response["status"] = "error";
            response["detail"] = "unsupported_action";
        } else {
            response["status"] = "accepted";
            response["detail"] = "worker_required";
        }

        String jsonString;
        serializeJson(response, jsonString);
        Webserver_sendata(jsonString);
    }
}
