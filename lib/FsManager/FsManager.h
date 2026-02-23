#ifndef FS_MANAGER_H
#define FS_MANAGER_H

#include <LittleFS.h>
#include <ArduinoJson.h>

bool fsInit();
String fsListJson(const String &path);
bool fsDelete(const String &path);

#endif