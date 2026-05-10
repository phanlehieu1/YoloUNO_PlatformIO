#include "global.h"

#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
// #include "mainserver.h"
// #include "tinyml.h"
#include "coreiot.h"

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

  check_info_File(0);

  // Task 1: LED blinks at speed determined by temperature condition
  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);

  // Task 2: NeoPixel colour determined by humidity level
  xTaskCreate(neo_blinky, "Task NEO Blink", 2048, NULL, 2, NULL);

  // Sensor task: reads DHT20 and notifies Task 1, 2, 3
  xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 4096, NULL, 2, NULL);

  // Task 3: LCD displays temp/humidity with NORMAL/WARNING/CRITICAL states
  xTaskCreate(lcd_display_task, "Task LCD Display", 4096, NULL, 2, NULL);

  // CoreIOT: publishes telemetry over MQTT
  xTaskCreate(coreiot_task, "CoreIOT Task", 4096, NULL, 2, NULL);

  // xTaskCreate(main_server_task, "Task Main Server", 8192, NULL, 2, NULL);
  // xTaskCreate(tiny_ml_task, "Tiny ML Task", 2048, NULL, 2, NULL);
  // xTaskCreate(Task_Toogle_BOOT, "Task_Toogle_BOOT", 4096, NULL, 2, NULL);
}

void loop()
{
  if (check_info_File(1))
  {
    if (!Wifi_reconnect())
    {
      Webserver_stop();
    }
    else
    {
      //CORE_IOT_reconnect();
    }
  }
  Webserver_reconnect();
}