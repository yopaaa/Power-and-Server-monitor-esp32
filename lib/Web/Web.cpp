#include "Web.h"
#include "./SavedWifi.h"
#include "Lcd_api.h"
#include "Wifi_api.h"
#include "index_html.h"
#include "LCD.h"
#include "PZEMManager.h"
#include "ServerMonitor.h"
#include <Update.h>
#include <LittleFS.h>

WebServer server(80);

static bool _pendingReboot = false;
static size_t _otaBytesWritten = 0;

static void addCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Content-Length");
}

static void handleUpdatePost() {
    addCorsHeaders();
    server.sendHeader("Connection", "close");

    if (Update.hasError()) {
        Update.printError(Serial);
        server.send(500, "text/plain", "Flash GAGAL! Firmware tidak valid atau rusak.");
    } else {
        server.send(200, "text/plain", "Flash SUKSES! ESP32 akan reboot...");
        _pendingReboot = true;
    }
}

static void handleUpdateUpload() {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        _otaBytesWritten = 0;
        Serial.printf("[WebOTA] Upload mulai: %s\n", upload.filename.c_str());
        drawOtaStart();

        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        size_t written = Update.write(upload.buf, upload.currentSize);
        if (written != upload.currentSize) {
            Update.printError(Serial);
        }

        _otaBytesWritten += upload.currentSize;

        int totalLength = server.header("Content-Length").toInt();
        if (totalLength > 0) {
            int percent = (_otaBytesWritten * 100) / totalLength;
            if (percent > 99) percent = 99;
            drawOtaProgress(percent);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("[WebOTA] Upload selesai! Total: %u bytes.\n", upload.totalSize);
            drawOtaProgress(100);
            drawOtaSuccess();
        } else {
            Update.printError(Serial);
            drawOtaError("Verifikasi gagal!");
        }
    }
}

void handleWebReboot() {
    if (_pendingReboot) {
        _pendingReboot = false;
        delay(1000);
        ESP.restart();
    }
}

void setupWeb()
{
    Serial.println("Starting web server");

    // Tangkap header Content-Length untuk perhitungan progress bar
    const char* headerkeys[] = {"Content-Length"};
    server.collectHeaders(headerkeys, 1);

    server.on("/", HTTP_GET, []() {
        Serial.println("HTTP GET /");
        String html = String(INDEX_HTML);
        server.send(200, "text/html", html);
    });

    server.on("/sys/info", HTTP_GET, []() {
        String json = "{";

        uint64_t chipid = ESP.getEfuseMac();
        json += "\"chipId\":\"" + String((uint32_t)(chipid >> 32), HEX) +
                String((uint32_t)chipid, HEX) + "\",";
        json += "\"flashSize\":" + String(ESP.getFlashChipSize()) + ",";
        json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"cpuFreqMHz\":" + String(ESP.getCpuFreqMHz()) + ",";
        json += "\"uptimeMs\":" + String(millis()) + ",";
        json += "\"localIP\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
        json += "\"wifiMode\":" + String(WiFi.getMode()) + ",";
        json += "\"wifiStatus\":" + String(WiFi.status()) + ",";

        const PZEMMetrics &m = getPZEMMetrics();
        json += "\"pzem\":{";
        json += "\"connected\":" + String(m.isConnected ? "true" : "false") + ",";
        json += "\"voltage\":" + String(m.voltage, 1) + ",";
        json += "\"current\":" + String(m.current, 2) + ",";
        json += "\"power\":" + String(m.power, 1) + ",";
        json += "\"energy\":" + String(m.energy, 2) + ",";
        json += "\"frequency\":" + String(m.frequency, 1) + ",";
        json += "\"pf\":" + String(m.pf, 2);
        json += "},";

        json += "\"otaReady\":true";
        json += "}";
        server.send(200, "application/json", json);
    });

    server.on("/pzem/reset", HTTP_POST, []() {
        resetPZEMEnergy();
        server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"PZEM energy counter reset\"}");
    });

    server.on("/ota", HTTP_GET, []() {
        server.send(200, "text/plain", "Web OTA active via POST /update or root page");
    });

    server.on("/lcd/invert", HTTP_GET, []() {
        lcdToggleInversion();
        server.send(200, "application/json", String("{\"inverted\":") + (lcdIsInverted() ? "true" : "false") + "}");
    });

    server.on("/lcd/brightness", HTTP_GET, []() {
        if (server.hasArg("val")) {
            int b = server.arg("val").toInt();
            lcdBacklight(b);
            server.send(200, "application/json", "{\"status\":\"ok\",\"brightness\":" + String(b) + "}");
        } else {
            server.send(400, "application/json", "{\"error\":\"Missing val param (0-255)\"}");
        }
    });

    server.on("/lcd/page", HTTP_GET, []() {
        if (server.hasArg("val")) {
            String val = server.arg("val");
            if (val == "server") {
                setDisplayPage(PAGE_SERVER_MONITOR);
            } else if (val == "power") {
                setDisplayPage(PAGE_POWER_METER);
            }
        } else {
            cycleDisplayPage();
        }
        server.send(200, "application/json", String("{\"page\":\"") + (getDisplayPage() == PAGE_SERVER_MONITOR ? "server" : "power") + "\"}");
    });

    server.on("/lcd/next", HTTP_GET, []() {
        cycleNextScreen();
        int idx = getScreenIndex();
        String name = (idx == 0) ? "Power Meter" : getCurrentServer().name;
        server.send(200, "application/json", "{\"screenIndex\":" + String(idx) + ",\"name\":\"" + name + "\"}");
    });

    server.on("/lcd/prev", HTTP_GET, []() {
        cyclePrevScreen();
        int idx = getScreenIndex();
        String name = (idx == 0) ? "Power Meter" : getCurrentServer().name;
        server.send(200, "application/json", "{\"screenIndex\":" + String(idx) + ",\"name\":\"" + name + "\"}");
    });

    server.on("/server/next", HTTP_GET, []() {
        nextServer();
        if (getDisplayPage() == PAGE_SERVER_MONITOR) {
            drawServerMonitorFrame(getCurrentServer(), getCurrentServerIndex(), getServerCount());
        }
        server.send(200, "application/json", String("{\"serverIndex\":") + getCurrentServerIndex() + ",\"name\":\"" + getCurrentServer().name + "\"}");
    });

    server.on("/server/prev", HTTP_GET, []() {
        prevServer();
        if (getDisplayPage() == PAGE_SERVER_MONITOR) {
            drawServerMonitorFrame(getCurrentServer(), getCurrentServerIndex(), getServerCount());
        }
        server.send(200, "application/json", String("{\"serverIndex\":") + getCurrentServerIndex() + ",\"name\":\"" + getCurrentServer().name + "\"}");
    });

    server.on("/server/list", HTTP_GET, []() {
        String json = "{\"servers\":[";
        for (int i = 0; i < getServerCount(); i++) {
            if (i > 0) json += ",";
            selectServer(i);
            const ServerMetrics &s = getCurrentServer();
            json += "{\"name\":\"" + s.name + "\",\"host\":\"" + s.host + "\",\"cpu\":" + String(s.cpuPercent, 1) + ",\"temp\":" + String(s.cpuTemp, 0) + ",\"ram\":" + String(s.ramPercent, 1) + ",\"disk\":" + String(s.diskPercent, 1) + "}";
        }
        json += "],\"current\":" + String(getCurrentServerIndex()) + "}";
        server.send(200, "application/json", json);
    });

    server.on("/sys/format", HTTP_GET, []() {
        Serial.println("[FS] Formatting LittleFS partition...");
        lcdClear(TFT_BLACK);
        lcdPrintCenterX("Formatting FS...", 2, TFT_ORANGE, 2);

        bool ok = false;
        if (LittleFS.begin(true)) {
            ok = LittleFS.format();
        }

        if (ok) {
            Serial.println("[FS] LittleFS formatted successfully");
            lcdPrintCenterX("FS Formatted OK", 4, TFT_GREEN, 2);
            server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"LittleFS partition formatted successfully\"}");
        } else {
            Serial.println("[FS] LittleFS format failed");
            lcdPrintCenterX("Format Failed", 4, TFT_RED, 2);
            server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to format LittleFS\"}");
        }
    });

    // Custom Web OTA Route
    server.on("/update", HTTP_POST, handleUpdatePost, handleUpdateUpload);
    server.on("/update", HTTP_OPTIONS, []() {
        addCorsHeaders();
        server.send(200);
    });

    setupWifiApi(server);
    setupLcdApi(server);

    server.begin();
    Serial.println("Web server running");
}


