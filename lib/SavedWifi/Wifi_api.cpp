#include "Wifi_api.h"
#include "SavedWifi.h"
#include "index_html.h"

static void addCorsHeaders(WebServer &server) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Content-Length");
}

void setupWifiApi(WebServer &server)
{
    server.on("/welcome", HTTP_GET, [&server]() {
        addCorsHeaders(server);
        String html = String(INDEX_HTML);
        server.send(200, "text/html", html);
    });

    // OPTIONS preflights
    server.on("/wifi/data", HTTP_OPTIONS, [&server]() {
        addCorsHeaders(server);
        server.send(200);
    });
    server.on("/wifi/add", HTTP_OPTIONS, [&server]() {
        addCorsHeaders(server);
        server.send(200);
    });
    server.on("/wifi/delete", HTTP_OPTIONS, [&server]() {
        addCorsHeaders(server);
        server.send(200);
    });
    server.on("/wifi/scan", HTTP_OPTIONS, [&server]() {
        addCorsHeaders(server);
        server.send(200);
    });
    server.on("/wifi/test", HTTP_OPTIONS, [&server]() {
        addCorsHeaders(server);
        server.send(200);
    });

    server.on("/wifi/scan", HTTP_GET, [&server]() {
        addCorsHeaders(server);
        Serial.println("[WiFi] Scanning available networks...");
        int n = WiFi.scanNetworks(false, true);
        String json = "{\"networks\":[";
        for (int i = 0; i < n; i++) {
            if (i > 0) json += ",";
            json += "{";
            json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
            json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
            json += "\"channel\":" + String(WiFi.channel(i)) + ",";
            json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
            json += "}";
        }
        json += "]}";
        server.send(200, "application/json", json);
    });

    server.on("/wifi/test", HTTP_POST, [&server]() {
        addCorsHeaders(server);
        if (!server.hasArg("ssid") || server.arg("ssid").isEmpty()) {
            server.send(400, "application/json",
                        "{\"ok\":false,\"message\":\"SSID wajib diisi\"}");
            return;
        }

        String ssid = server.arg("ssid");
        ssid.trim();
        String pass = server.hasArg("pass") ? server.arg("pass") : "";
        int channel = server.hasArg("channel") ? server.arg("channel").toInt() : 0;

        Serial.printf("[WiFi Test] Testing connection to '%s'...\n", ssid.c_str());

        // Jika channel belum diketahui, cari dari cache scan
        if (channel <= 0) {
            int n = WiFi.scanComplete();
            if (n > 0) {
                for (int i = 0; i < n; i++) {
                    if (WiFi.SSID(i) == ssid) {
                        channel = WiFi.channel(i);
                        break;
                    }
                }
            }
        }

        if (channel > 0) {
            WiFi.begin(ssid.c_str(), pass.c_str(), channel);
        } else {
            WiFi.begin(ssid.c_str(), pass.c_str());
        }

        unsigned long start = millis();

        while (WiFi.status() != WL_CONNECTED && (millis() - start < 10000)) {
            delay(100);
            server.handleClient();
            if (WiFi.status() == WL_CONNECT_FAILED) {
                break;
            }
        }

        if (WiFi.status() != WL_CONNECTED) {
            addCorsHeaders(server);
            server.send(200, "application/json", "{\"ok\":false,\"message\":\"Gagal terhubung ke WiFi. Periksa password atau sinyal hotspot.\"}");
            return;
        }

        IPAddress ip = WiFi.localIP();
        IPAddress gw = WiFi.gatewayIP();
        IPAddress sn = WiFi.subnetMask();

        Serial.printf("[WiFi Test] Connected! IP: %s, GW: %s, SN: %s\n",
                      ip.toString().c_str(), gw.toString().c_str(), sn.toString().c_str());

        String json = "{";
        json += "\"ok\":true,";
        json += "\"ip\":\"" + ip.toString() + "\",";
        json += "\"gw\":\"" + gw.toString() + "\",";
        json += "\"sn\":\"" + sn.toString() + "\",";
        json += "\"message\":\"Berhasil terhubung ke " + ssid + "!\"";
        json += "}";

        // Kirim respon sebelum melakukan aksi apapun
        addCorsHeaders(server);
        server.send(200, "application/json", json);
    });

    server.on("/wifi/data", HTTP_GET, [&server]() {
        addCorsHeaders(server);
        String json = "{";
        json += "\"wifiList\":[";

        for (int i = 0; i < wifiCount; i++) {
            json += "{";
            json += "\"ssid\":\"" + wifiList[i].ssid + "\",";
            json += "\"useStatic\":" +
                    String(wifiList[i].useStatic ? "true" : "false") + ",";
            json += "\"ip\":\"" + wifiList[i].ip.toString() + "\",";
            json += "\"gateway\":\"" + wifiList[i].gw.toString() + "\",";
            json += "\"subnet\":\"" + wifiList[i].sn.toString() + "\"";
            json += "}";

            if (i < wifiCount - 1)
                json += ",";
        }

        json += "]}";

        server.send(200, "application/json", json);
    });

    server.on("/wifi/add", HTTP_POST, [&server]() {
        addCorsHeaders(server);
        if (!server.hasArg("ssid") || server.arg("ssid").isEmpty()) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID wajib diisi\"}");
            return;
        }

        String ssid = server.arg("ssid");
        ssid.trim();
        if (ssid.isEmpty()) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID tidak boleh kosong\"}");
            return;
        }

        String pass = server.hasArg("pass") ? server.arg("pass") : "";

        bool useStatic = server.hasArg("static");

        IPAddress ip, gw, sn;
        if (useStatic) {
            ip.fromString(server.arg("ip"));
            gw.fromString(server.arg("gw"));
            sn.fromString(server.arg("sn"));
        }

        addOrUpdateWiFi(ssid, pass, useStatic, ip, gw, sn);

        server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"WiFi berhasil disimpan\"}");
    });

    server.on("/wifi/delete", HTTP_POST, [&server]() {
        addCorsHeaders(server);
        if (!server.hasArg("ssid") || server.arg("ssid").isEmpty()) {
            server.send(
                400, "application/json",
                "{\"status\":\"error\",\"message\":\"SSID wajib diisi\"}");
            return;
        }

        String ssid = server.arg("ssid");
        if (deleteWiFi(ssid))
            server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"WiFi berhasil dihapus\"}");
        else
            server.send(
                404, "application/json",
                "{\"status\":\"error\",\"message\":\"SSID tidak ditemukan\"}");
    });
}