#include "SavedWifi.h"
#include "Web.h"
#include <ESP8266mDNS.h>
#include <ElegantOTA.h>

#include "FsManager.h"
#include "LCD.h"
#include "Lcd_api.h"

#include "Analog_clock.h"
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
#if defined(ESP8266)
    if (WiFi.status() != WL_CONNECTED) {
        MDNS.update();
    }
#endif
    server.handleClient();
    // ElegantOTA.loop();
    lcdApiLoop();
    if (otaEnabled) {
        ElegantOTA.loop();
    }

    updateClock();
}


