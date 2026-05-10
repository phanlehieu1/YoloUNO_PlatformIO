#include "task_wifi.h"

static bool staConnected = false;

void startAP()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(String(SSID_AP), String(PASS_AP));
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void initAPSTA()
{
    // Called once from setup() — starts AP immediately so the config portal
    // is accessible, then kicks off STA connection in the background.
    const DeviceConfig config = getDeviceConfig();

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(String(SSID_AP), String(PASS_AP));
    Serial.print("AP+STA IP: ");
    Serial.println(WiFi.softAPIP());

    if (!config.wifiSsid.isEmpty())
    {
        WiFi.setAutoReconnect(true);
        if (config.wifiPass.isEmpty())
            WiFi.begin(config.wifiSsid.c_str());
        else
            WiFi.begin(config.wifiSsid.c_str(), config.wifiPass.c_str());
        Serial.println("[WiFi] STA connecting in background...");
    }
}

bool Wifi_reconnect()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!staConnected)
        {
            staConnected = true;
            Serial.print("Station IP: ");
            Serial.println(WiFi.localIP());
            xSemaphoreGive(internetConnectedSemaphore());
        }
        return true;
    }
    return false;
}
