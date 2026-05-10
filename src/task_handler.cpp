#include <task_handler.h>
#include <task_webserver.h>

void handleWebSocketMessage(String message)
{
    Serial.println(message);
    StaticJsonDocument<256> doc;

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

        Serial.printf("⚙️ Điều khiển GPIO %d → %s\n", gpio, status.c_str());
        pinMode(gpio, OUTPUT);
        if (status.equalsIgnoreCase("ON"))
        {
            digitalWrite(gpio, HIGH);
            Serial.printf("🔆 GPIO %d ON\n", gpio);
        }
        else if (status.equalsIgnoreCase("OFF"))
        {
            digitalWrite(gpio, LOW);
            Serial.printf("💤 GPIO %d OFF\n", gpio);
        }
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
}
