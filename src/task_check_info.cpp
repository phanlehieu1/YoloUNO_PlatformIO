#include "task_check_info.h"

void Load_info_File()
{
  File file = LittleFS.open("/info.dat", "r");
  if (!file)
  {
    return;
  }
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
  }
  else
  {
    AppConfig &config = appConfig();
    config.wifiSsid = doc["WIFI_SSID"].as<String>();
    config.wifiPass = doc["WIFI_PASS"].as<String>();
    config.coreIotToken = doc["CORE_IOT_TOKEN"].as<String>();
    config.coreIotServer = doc["CORE_IOT_SERVER"].as<String>();
    config.coreIotPort = doc["CORE_IOT_PORT"].as<String>();
  }
  file.close();
}

void Delete_info_File()
{
  if (LittleFS.exists("/info.dat"))
  {
    LittleFS.remove("/info.dat");
  }
  ESP.restart();
}

void Save_info_File(String wifi_ssid, String wifi_pass, String core_iot_token, String core_iot_server, String core_iot_port)
{
  Serial.println(wifi_ssid);
  Serial.println(wifi_pass);

  DynamicJsonDocument doc(4096);
  doc["WIFI_SSID"] = wifi_ssid;
  doc["WIFI_PASS"] = wifi_pass;
  doc["CORE_IOT_TOKEN"] = core_iot_token;
  doc["CORE_IOT_SERVER"] = core_iot_server;
  doc["CORE_IOT_PORT"] = core_iot_port;

  File configFile = LittleFS.open("/info.dat", "w");
  if (configFile)
  {
    serializeJson(doc, configFile);
    configFile.close();
  }
  else
  {
    Serial.println("Unable to save the configuration.");
  }
  ESP.restart();
};

bool check_info_File(bool check)
{
  if (!check)
  {
    if (!LittleFS.begin(true))
    {
      Serial.println("❌ Lỗi khởi động LittleFS!");
      return false;
    }
    Load_info_File();
  }
  
  AppConfig &config = appConfig();
  if (config.wifiSsid.isEmpty() && config.wifiPass.isEmpty())
  {
    if (!check)
    {
      startAP();
    }
    return false;
  }
  return true;
}