#include "SavedWifi.h"
#include "Web.h"
#include <ESPmDNS.h>
#include <ElegantOTA.h>

#include "FsManager.h"
#include "LCD.h"
#include "Lcd_api.h"

#include "Analog_clock.h"

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // GPIO 2 adalah standar LED pada kebanyakan ESP32
#endif

void setup()
{
    Serial.begin(9600);
    fsInit();
    lcdInit();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // loadConfig();
    loadWiFiList();

    WiFi.mode(WIFI_STA);

    if (!connectSavedWiFi()) {
        startAP();
    }

    setupWeb();

    // lcdClear();
    lcdPrint("HTTP server started", 10, 40, TFT_RED, 2);

    drawDial();
}

void loop()
{
    server.handleClient();
    // ElegantOTA.loop();
    lcdApiLoop();
    if (otaEnabled) {
        ElegantOTA.loop();
    }

    updateClock();
}