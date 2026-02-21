#include "FsApi.h"
#include "FsManager.h"
#include "fs_html.h"

void handleUpload(ESP8266WebServer &server)
{
    HTTPUpload& upload = server.upload();

    static File uploadFile;

    if (upload.status == UPLOAD_FILE_START) {
        String filename = "/" + upload.filename;
        uploadFile = LittleFS.open(filename, "w");
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile)
            uploadFile.write(upload.buf, upload.currentSize);
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile)
            uploadFile.close();
    }
}

void setupFsApi(ESP8266WebServer &server)
{
    server.on("/fs", HTTP_GET, [&server]() {
        String html = String(INDEX_HTML);
        server.send(200, "text/html", html);
    });

    // LIST FILES
    server.on("/fs/list", HTTP_GET, [&server]() {
        server.send(200, "application/json", fsListJson());
    });

    server.on("/fs/info", HTTP_GET, [&server]() {

        FSInfo fs_info;
        LittleFS.info(fs_info);

        size_t total = fs_info.totalBytes;
        size_t used  = fs_info.usedBytes;
        size_t free  = total - used;

        String json = "{";
        json += "\"total\":" + String(total) + ",";
        json += "\"used\":"  + String(used)  + ",";
        json += "\"free\":"  + String(free)  + ",";
        json += "\"usedPercent\":" + String((used * 100) / total);
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
}