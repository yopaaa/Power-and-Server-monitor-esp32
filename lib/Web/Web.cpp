#include "Web.h"
#include "./SavedWifi.h"
#include "FsApi.h"
#include "Lcd_api.h"
#include "Wifi_api.h"
#include "index_html.h"
#include "FsManager.h"
#include <ElegantOTA.h>

WebServer server(80);

void onOTAStart() { Serial.println("OTA started"); }

void onOTAEnd(bool success)
{
    if (success)
        Serial.println("OTA success");
    else
        Serial.println("OTA failed");
}

bool otaInitialized = false;

void enableOTA()
{
    if (otaInitialized)
        return;

    ElegantOTA.begin(&server);
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onEnd(onOTAEnd);

    otaInitialized = true;
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

    server.on("/sys/info", HTTP_GET, []() {
        String json = "{";

        uint64_t chipid = ESP.getEfuseMac();
        json += "\"chipId\":\"" + String((uint32_t)(chipid >> 32), HEX) +
                String((uint32_t)chipid, HEX) + "\",";
        json += "\"flashSize\":" + String(ESP.getFlashChipSize()) + ",";
        json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"fsTotal\":" + String(LittleFS.totalBytes()) + ",";
        json += "\"fsUsed\":" + String(LittleFS.usedBytes()) + ",";
        json += "\"fsFree\":" +
                String(LittleFS.totalBytes() - LittleFS.usedBytes()) + ",";

        json += "\"cpuFreqMHz\":" + String(ESP.getCpuFreqMHz()) + ",";
        json += "\"uptimeMs\":" + String(millis()) + ",";
        json += "\"localIP\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
        json += "\"wifiMode\":" + String(WiFi.getMode()) + ",";

        json += "\"wifiStatus\":" + String(WiFi.status()) + ",";

        json += "\"localIP\":\"" + WiFi.localIP().toString() + "\",";

        json += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\",";
        json += "\"otaEnabled\":" + String(otaEnabled ? 1 : 0);

        json += "}";
        server.send(200, "application/json", json);
    });

    server.on("/ota", HTTP_GET, []() {
        enableOTA();
        otaEnabled = true;
        server.send(200, "text/plain", "OTA enabled");
    });

    setupFsApi(server);
    setupWifiApi(server);
    setupLcdApi(server);

    server.begin();
    Serial.println("Web server running");
}
