#include "task_webserver.h"
#include <task_handler.h>

namespace {
AsyncWebServer &webServer()
{
    static AsyncWebServer server(80);
    return server;
}

AsyncWebSocket &webSocket()
{
    static AsyncWebSocket ws("/ws");
    return ws;
}

bool &webserverRunning()
{
    static bool running = false;
    return running;
}
}

void Webserver_sendata(String data)
{
    if (webSocket().count() > 0)
    {
        webSocket().textAll(data); // Gửi đến tất cả client đang kết nối
        Serial.println("📤 Đã gửi dữ liệu qua WebSocket: " + data);
    }
    else
    {
        Serial.println("⚠️ Không có client WebSocket nào đang kết nối!");
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;

        if (info->opcode == WS_TEXT)
        {
            String message;
            message += String((char *)data).substring(0, len);
            handleWebSocketMessage(message);
        }
    }
}

void connnectWSV()
{
    webSocket().onEvent(onEvent);
    webServer().addHandler(&webSocket());
    webServer().on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/index.html", "text/html"); });
    webServer().on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/script.js", "application/javascript"); });
    webServer().on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/styles.css", "text/css"); });
    webServer().begin();
    ElegantOTA.begin(&webServer());
    webserverRunning() = true;
}

void Webserver_stop()
{
    webSocket().closeAll();
    webServer().end();
    webserverRunning() = false;
}

void Webserver_reconnect()
{
    if (!webserverRunning())
    {
        connnectWSV();
    }
    ElegantOTA.loop();
}
