#include "SavedWifi.h"
#include "Web.h"
#include <ESPmDNS.h>
#include <ElegantOTA.h>

#include "FsManager.h"
#include "LCD.h"
#include "Lcd_api.h"

#include "Services.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 
#endif

void setup()
{
    Serial.begin(9600);
    fsInit();
    lcdInit();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    loadWiFiList();

    WiFi.mode(WIFI_STA);

    if (!connectSavedWiFi()) {
        startAP();
    }

    setupWeb();

    lcdPrint("HTTP server started", 10, 40, TFT_RED, 2);

    loadService();
}

void loop()
{
    server.handleClient();
    if (otaEnabled) {
        ElegantOTA.loop();
    }

    servicesLoop();
}