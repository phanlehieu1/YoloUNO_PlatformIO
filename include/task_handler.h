
#ifndef __TASK_HANDLER_H__
#define __TASK_HANDLER_H__

#include <ArduinoJson.h>
#include <task_check_info.h>
#include "RPC_Callback.h"

extern void handleWebSocketMessage(String message);
bool applyDevicePowerCommand(int gpio, const String &status);
RPC_Response handlePowerRpc(const JsonVariantConst &data);
#endif