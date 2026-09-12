#include "SavedWifi.h"
#include "Web.h"
#include <ESPmDNS.h>

#include "LCD.h"
#include "Lcd_api.h"
#include "Devices.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 
#endif

void setup()
{
    Serial.begin(115200);
    lcdInit();
    initDeviceID();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    loadWiFiList();

    WiFi.mode(WIFI_STA);

    if (!connectSavedWiFi()) {
        startAP();
    }

    setupWeb();
    initUDPDiscovery();

    lcdClear(TFT_BLACK);
    lcdPrintCenterX("Power Meter", 1, TFT_YELLOW, 2);
    lcdPrintCenterX("WiFi Connected", 3, TFT_GREEN, 2);
    lcdPrintCenterX(WiFi.localIP().toString(), 4, TFT_CYAN, 2);
}

void loop()
{
    server.handleClient();
    handleWebReboot();
    handleUDPDiscovery();
}