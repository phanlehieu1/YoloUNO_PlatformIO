#include "global.h"

#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "tinyml.h"

#include "task_check_info.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"
#include <WiFi.h>

namespace {
#if defined(DEVICE_ROLE_ACTUATOR)
void startActuatorServices() {
  initRtosResources();
  check_info_File(0);

  DeviceConfig config = getDeviceConfig();
  config.wifiSsid = String(DEVICE_WIFI_SSID);
  config.wifiPass = String(DEVICE_WIFI_PASS);
  config.coreIotToken = String(DEVICE_CORE_IOT_TOKEN);
  setDeviceConfig(config);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(config.wifiSsid.c_str(), config.wifiPass.c_str());
  Serial.printf("[Setup] Actuator STA connecting to %s\n", config.wifiSsid.c_str());

  Webserver_reconnect();
  Serial.println("[Setup] Actuator Web Server is READY for OTA.");
}
#else
void startCommonServices() {
  initRtosResources();
  check_info_File(0);

  if (hasWifiCredentials()) {
    initAPSTA();
  } else {
    startAP();
  }

  Webserver_reconnect();
  Serial.println("[Setup] AP and Web Server are READY.");
}
#endif
} // namespace

void setup()
{
  Serial.begin(115200);
  delay(1500);
  Serial.println("[Setup] Boot start");
  Serial.printf("[Setup] Firmware Version: %s\n", getFirmwareVersion().c_str());

#if defined(DEVICE_ROLE_ACTUATOR)
  startActuatorServices();
  Serial.println("[Setup] Role = ACTUATOR");
  xTaskCreate(coreiot_task, "CoreIOT Task", 6144, NULL, 2, NULL);
#else
  startCommonServices();
  Serial.println("[Setup] Role = SENSOR");
  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);
  xTaskCreate(neo_blinky, "Task NEO Blink", 4096, NULL, 2, NULL);
  xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 4096, NULL, 2, NULL);
  xTaskCreate(lcd_display_task, "Task LCD Display", 4096, NULL, 2, NULL);
  xTaskCreate(tiny_ml_task, "TinyML Task", 8192, NULL, 2, NULL);
  xTaskCreate(coreiot_task, "CoreIOT Task", 6144, NULL, 2, NULL);
#endif
}

void loop()
{
#if defined(DEVICE_ROLE_ACTUATOR)
  Wifi_reconnect();
  Webserver_reconnect();
#else
  if (hasWifiCredentials()) {
    Wifi_reconnect();
  }
  Webserver_reconnect();
#endif
}