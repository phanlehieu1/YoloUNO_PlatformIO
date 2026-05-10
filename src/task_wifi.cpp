#include "task_wifi.h"

void startAP()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(String(SSID_AP), String(PASS_AP));
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void startSTA()
{
    const DeviceConfig config = getDeviceConfig();
    if (config.wifiSsid.isEmpty())
    {
        vTaskDelete(NULL);
    }

    WiFi.mode(WIFI_STA);

    if (config.wifiPass.isEmpty())
    {
        WiFi.begin(config.wifiSsid.c_str());
    }
    else
    {
        WiFi.begin(config.wifiSsid.c_str(), config.wifiPass.c_str());
    }

    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // This semaphore releases network-dependent tasks (CoreIOT) after Wi-Fi connects.
    xSemaphoreGive(internetConnectedSemaphore());
}

bool Wifi_reconnect()
{
    const wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED)
    {
        return true;
    }
    startSTA();
    return false;
}
