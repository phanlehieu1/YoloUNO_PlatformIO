
#include "task_core_iot.h"

namespace {
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr char LED_STATE_ATTR[] = "ledState";
constexpr std::array<const char *, 1U> SHARED_ATTRIBUTES_LIST = {
    LED_STATE_ATTR,
};

WiFiClient &thingsBoardWifiClient()
{
    static WiFiClient client;
    return client;
}

Arduino_MQTT_Client &thingsBoardMqttClient()
{
    static Arduino_MQTT_Client client(thingsBoardWifiClient());
    return client;
}

ThingsBoard &thingsBoardClient()
{
    static ThingsBoard tb(thingsBoardMqttClient(), MAX_MESSAGE_SIZE);
    return tb;
}
}

void processSharedAttributes(const Shared_Attribute_Data &data)
{
    for (auto it = data.begin(); it != data.end(); ++it)
    {
        (void)it;
    }
}

RPC_Response setLedSwitchValue(const RPC_Data &data)
{
    Serial.println("Received Switch state");
    bool newState = data;
    Serial.print("Switch state change: ");
    Serial.println(newState);
    return RPC_Response("setLedSwitchValue", newState);
}

const std::array<RPC_Callback, 1U> &rpcCallbacks()
{
    static const std::array<RPC_Callback, 1U> callbacks = {
        RPC_Callback{"setLedSwitchValue", setLedSwitchValue}};
    return callbacks;
}

const Shared_Attribute_Callback &attributesCallback()
{
    static const Shared_Attribute_Callback callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
    return callback;
}

const Attribute_Request_Callback &attributeSharedRequestCallback()
{
    static const Attribute_Request_Callback callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
    return callback;
}

void CORE_IOT_sendata(String mode, String feed, String data)
{
    if (mode == "attribute")
    {
        thingsBoardClient().sendAttributeData(feed.c_str(), data);
    }
    else if (mode == "telemetry")
    {
        float value = data.toFloat();
        thingsBoardClient().sendTelemetryData(feed.c_str(), value);
    }
    else
    {
        // Unknown modes are ignored so invalid RPC payloads cannot block the MQTT task.
    }
}

void CORE_IOT_reconnect()
{
    ThingsBoard &tb = thingsBoardClient();
    if (!tb.connected())
    {
        const DeviceConfig config = getDeviceConfig();
        if (!tb.connect(config.coreIotServer.c_str(), config.coreIotToken.c_str(), config.coreIotPort.toInt()))
        {
            return;
        }

        tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());

        Serial.println("Subscribing for RPC...");
        if (!tb.RPC_Subscribe(rpcCallbacks().cbegin(), rpcCallbacks().cend()))
        {
            return;
        }

        if (!tb.Shared_Attributes_Subscribe(attributesCallback()))
        {
            return;
        }

        Serial.println("Subscribe done");

        if (!tb.Shared_Attributes_Request(attributeSharedRequestCallback()))
        {
            return;
        }
        tb.sendAttributeData("localIp", WiFi.localIP().toString().c_str());
    }
    else if (tb.connected())
    {
        tb.loop();
    }
}
