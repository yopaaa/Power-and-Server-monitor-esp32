#include "FsApi.h"
#include "FsManager.h"
#include "fs_html.h"

void handleUpload(WebServer &server)
{
    HTTPUpload& upload = server.upload();
    static File uploadFile;

    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        // Pastikan filename dimulai dengan /
        if (!filename.startsWith("/")) filename = "/" + filename;
        
        Serial.print("Uploading: "); Serial.println(filename);
        
        // Membuka file untuk ditulis (ini akan menimpa file lama jika namanya sama)
        uploadFile = LittleFS.open(filename, "w");
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
            Serial.println("Upload Finished");
        }
    }
}

void setupFsApi(WebServer &server)
{
    server.on("/fs", HTTP_GET, [&server]() {
        String html = String(INDEX_HTML);
        server.send(200, "text/html", html);
    });

    // LIST FILES
    server.on("/fs/list", HTTP_GET, [&server]() {
        if (!server.hasArg("path")) {
            server.send(400, "application/json", "{\"error\":\"path required\"}");
            return;
        }

        String path = server.arg("path");
        server.send(200, "application/json", fsListJson(path));
    });

    server.on("/fs/info", HTTP_GET, [&server]() {
        size_t total = LittleFS.totalBytes();
        size_t used  = LittleFS.usedBytes();
        size_t free  = total - used;

        String json = "{";
        json += "\"total\":" + String(total) + ",";
        json += "\"used\":"  + String(used)  + ",";
        json += "\"free\":"  + String(free)  + ",";
        // Proteksi division by zero jika FS belum mount
        json += "\"usedPercent\":" + String(total > 0 ? (used * 100) / total : 0);
        json += "}";

        server.send(200, "application/json", json);
    });

    // DELETE FILE
    server.on("/fs/delete", HTTP_POST, [&server]() {
        if (!server.hasArg("path")) {
            server.send(400, "application/json", "{\"error\":\"path required\"}");
            return;
        }

        String path = server.arg("path");

        if (fsDelete(path)) {
            server.send(200, "application/json", "{\"status\":\"deleted\"}");
        } else {
            server.send(404, "application/json", "{\"error\":\"not found\"}");
        }
    });

    // UPLOAD FILE
    server.on(
        "/fs/upload",
        HTTP_POST,
        [&server]() {
            server.send(200, "application/json", "{\"status\":\"uploaded\"}");
        },
        [&server]() {
            handleUpload(server);
        }
    );

    server.on("/fs/mkdir", HTTP_POST, [&server]() {
        if (!server.hasArg("path")) {
            server.send(400, "application/json", "{\"error\":\"path required\"}");
            return;
        }
        String path = server.arg("path");
        if (!path.startsWith("/")) path = "/" + path;

        if (LittleFS.mkdir(path)) {
            server.send(200, "application/json", "{\"status\":\"folder created\"}");
        } else {
            server.send(500, "application/json", "{\"error\":\"failed to create folder\"}");
        }
    });
}