#include "Web.h"
#include "./SavedWifi.h"
#include "index_html.h"
#include <ElegantOTA.h>
#include "FsApi.h"
#include "Wifi_api.h"
#include "Lcd_api.h"

ESP8266WebServer server(80);

void onOTAStart() { Serial.println("OTA started"); }

void onOTAEnd(bool success)
{
    if (success)
        Serial.println("OTA success");
    else
        Serial.println("OTA failed");
}

void setupWeb()
{
    Serial.println("Starting web server");

    server.on("/", HTTP_GET, []() {
        Serial.println("HTTP GET /config");

        String html = "<!doctype html> <html> <head>"
                      "<meta charset='utf-8'>"
                      "<title></title>"
                      "</head>"
                      "<body>"
                      "<script src='http://192.168.2.50/scriptt.js'></script>"
                      "</body>"
                      "</html>";
        server.send(200, "text/html", html);
    });
    
    setupFsApi(server);
    setupWifiApi(server);
    setupLcdApi(server);

    server.begin();
    Serial.println("Web server running");
    ElegantOTA.begin(&server);
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onEnd(onOTAEnd);
}
