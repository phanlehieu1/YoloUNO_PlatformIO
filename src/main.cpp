#include "global.h"

#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
// #include "mainserver.h"
#include "tinyml.h"
// #include "coreiot.h"

// include task
#include "task_check_info.h"
#include "task_toogle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"

void setup()
{
  Serial.begin(115200);

  // Create all RTOS resources (queues, mutexes, semaphores) before any task starts.
  initRtosResources();

  // 1. Initialize LittleFS and load saved config
  check_info_File(0);

  // 2. ALWAYS start AP + Web Server immediately so the config portal works
  //    If there are WiFi credentials, also start STA in the background.
  if (hasWifiCredentials()) {
    initAPSTA();   // AP + STA (non-blocking)
  } else {
    startAP();     // AP only
  }
  Webserver_reconnect();  // Start web server RIGHT AWAY
  Serial.println("[Setup] AP and Web Server are READY.");

  // 3. Create RTOS tasks
  // Task 1: LED blinks at speed determined by temperature condition
  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);

  // Task 2: NeoPixel colour determined by humidity level
  // Stack 4096: Adafruit NeoPixel library needs more than 2048
  xTaskCreate(neo_blinky, "Task NEO Blink", 4096, NULL, 2, NULL);

  // Sensor task: reads DHT20 and notifies Task 1, 2, 3
  xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 4096, NULL, 2, NULL);

  // Task 3: LCD displays temp/humidity with NORMAL/WARNING/CRITICAL states
  xTaskCreate(lcd_display_task, "Task LCD Display", 4096, NULL, 2, NULL);

  // Task 5: TinyML anomaly detection on sensor data
  xTaskCreate(tiny_ml_task, "TinyML Task", 8192, NULL, 2, NULL);

  // CoreIOT: publishes telemetry over MQTT
  xTaskCreate(coreiot_task, "CoreIOT Task", 4096, NULL, 2, NULL);
}

void loop()
{
  // Just poll WiFi status (never blocks, never resets AP)
  if (hasWifiCredentials()) {
    Wifi_reconnect();
  }
  Webserver_reconnect();
}