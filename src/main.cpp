#include "SavedWifi.h"
#include "Web.h"
#include <ESP8266mDNS.h>
#include <ElegantOTA.h>

#include "FsManager.h"
#include "LCD.h"



void setup()
{
    Serial.begin(9600);
    fsInit();
    lcdInit();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

   


    lcdClear();
    lcdPrint("Loading...", 10, 40, TFT_RED, 3);


    // loadConfig();
    loadWiFiList();

    WiFi.mode(WIFI_STA);

    if (!connectSavedWiFi()) {
        startAP();
    }

    setupWeb();

    tft.fillScreen(TFT_BLACK);
    digitalWrite(16, HIGH);

    lcdClear();
    lcdPrint("HTTP server started", 10, 40, TFT_RED, 2);
}

void loop()
{
#if defined(ESP8266)
    if (WiFi.status() != WL_CONNECTED) {
        MDNS.update();
    }
#endif
    server.handleClient();
    ElegantOTA.loop();
   
}


