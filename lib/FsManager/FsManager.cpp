#include "FsManager.h"

bool fsInit()
{
    // Jika mount gagal, ESP32 akan memformat partisi agar bisa digunakan.
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount gagal");
        return false;
    }
    Serial.println("LittleFS siap");
    return true;
}

String fsListJson()
{
    JsonDocument doc; 
    JsonArray arr = doc["files"].to<JsonArray>();

    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) {
        return "{\"files\":[]}";
    }

    File file = root.openNextFile();
    while (file)
    {
        JsonObject f = arr.add<JsonObject>();
        String name = String(file.name());
        if (name.startsWith("/")) name = name.substring(1); 
        
        f["name"] = name;
        f["size"] = file.size();
        
        file = root.openNextFile();
    }

    String output;
    serializeJson(doc, output);
    return output;
}

bool fsDelete(const String &path)
{
    String fullPath = path;
    if (!fullPath.startsWith("/")) {
        fullPath = "/" + fullPath;
    }

    if (!LittleFS.exists(fullPath)) {
        return false;
    }
    return LittleFS.remove(fullPath);
}