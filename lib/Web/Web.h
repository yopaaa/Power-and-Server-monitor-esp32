#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

extern Preferences prefs;
extern WebServer server;

void setupWeb();

#endif
