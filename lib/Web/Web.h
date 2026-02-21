#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H


#if defined(ESP8266)
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#elif defined(ESP32)
#include <WebServer.h>
#include <WiFi.h>
#include <Preferences.h>
#endif



#if defined(ESP32)
  extern Preferences prefs;
  extern WebServer server;
#elif defined(ESP8266)
  extern ESP8266WebServer server;
#endif


void setupWeb();

#endif
