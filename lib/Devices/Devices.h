#ifndef DEVICES_H
#define DEVICES_H

#include <Arduino.h>

#define VERSION "1.0.0"

extern String _id;
void initDeviceID();
void initUDPDiscovery();
void handleUDPDiscovery();

#endif