#ifndef SAVED_WIFI_H
#define SAVED_WIFI_H

#define MAX_WIFI 5

#include <WiFi.h>
#include <Preferences.h>


struct WiFiCred {
    String ssid;
    String pass;
    bool useStatic;
    IPAddress ip;
    IPAddress gw;
    IPAddress sn;
};

extern WiFiCred wifiList[MAX_WIFI];
extern uint8_t wifiCount;
extern Preferences prefs;

void loadWiFiList();
void saveWiFiList();
int scanWiFi(String found[], int max);
bool connectSavedWiFi();
void addOrUpdateWiFi(String ssid, String pass, bool useStatic, IPAddress ip,
                     IPAddress gw, IPAddress sn);
bool deleteWiFi(String ssid);

void startAP();

#endif