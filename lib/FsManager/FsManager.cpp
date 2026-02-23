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

String fsListJson(const String &path = "/")
{
    JsonDocument doc; 
    JsonArray arr = doc.to<JsonArray>(); 

    File root = LittleFS.open(path);
    if (!root || !root.isDirectory()) {
        return "[]"; 
    }

    File file = root.openNextFile();
    while (file)
    {
        JsonObject f = arr.add<JsonObject>();
        
        // Mengambil nama file saja (tanpa path lengkap jika di dalam subfolder)
        String fileName = String(file.name());
        
        // Membersihkan karakter '/' di awal jika ada
        if (fileName.startsWith("/")) {
            fileName = fileName.substring(1);
        }
        
        // Jika berada di dalam subfolder, file.name() terkadang mengembalikan path lengkap.
        // Kita hanya butuh nama akhirnya saja untuk tampilan tabel.
        int lastIndex = fileName.lastIndexOf('/');
        if (lastIndex != -1) {
            fileName = fileName.substring(lastIndex + 1);
        }

        f["name"] = fileName;
        f["size"] = file.size();
        f["isDir"] = file.isDirectory(); // Menambahkan status Folder/Direktori
        
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