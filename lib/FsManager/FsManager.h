#ifndef FS_MANAGER_H
#define FS_MANAGER_H

#include <LittleFS.h>
#include <ArduinoJson.h>

bool fsInit();
String fsListJson();
bool fsDelete(const String &path);

#endif