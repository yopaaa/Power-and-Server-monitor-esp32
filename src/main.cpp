#include "SavedWifi.h"
#include "Web.h"
#include <ESPmDNS.h>

#include "LCD.h"
#include "Lcd_api.h"
#include "Devices.h"
#include "PZEMManager.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 
#endif

static unsigned long lastPzemRead = 0;

void setup()
{
    Serial.begin(115200);
    lcdInit();
    initDeviceID();
    initPZEM();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    loadWiFiList();

    WiFi.mode(WIFI_STA);

    if (!connectSavedWiFi()) {
        startAP();
    }

    setupWeb();
    initUDPDiscovery();

    drawPowerMeterFrame();
}

void loop()
{
    server.handleClient();
    handleWebReboot();
    handleUDPDiscovery();

    unsigned long currentMillis = millis();
    if (currentMillis - lastPzemRead >= 1000) {
        lastPzemRead = currentMillis;
        readPZEM();

        String statusInfo = "";
        if (WiFi.status() == WL_CONNECTED) {
            statusInfo = WiFi.localIP().toString();
        } else if (WiFi.getMode() == WIFI_AP) {
            statusInfo = "AP: " + WiFi.softAPIP().toString();
        } else {
            statusInfo = "No WiFi";
        }

        updatePowerMeterDisplay(getPZEMMetrics(), statusInfo);
    }
}