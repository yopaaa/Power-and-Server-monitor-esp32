#include "FsManager.h"

bool fsInit()
{
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount gagal");
        return false;
    }
    Serial.println("LittleFS siap");
    return true;
}

String fsListJson()
{
    StaticJsonDocument<2048> doc;
    JsonArray arr = doc.createNestedArray("files");

    Dir dir = LittleFS.openDir("/");
    while (dir.next())
    {
        JsonObject f = arr.createNestedObject();
        f["name"] = dir.fileName();
        f["size"] = dir.fileSize();
    }

    String output;
    serializeJson(doc, output);
    return output;
}

bool fsDelete(const String &path)
{
    if (!LittleFS.exists(path)) {
        return false;
    }
    return LittleFS.remove(path);
}