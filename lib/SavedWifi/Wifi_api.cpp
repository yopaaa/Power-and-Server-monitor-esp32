#include "Wifi_api.h"
#include "SavedWifi.h"
#include "index_html.h"

void setupWifiApi(WebServer &server)
{
    server.on("/welcome", HTTP_GET, [&server]() {
        String html = String(INDEX_HTML);
        server.send(200, "text/html", html);
    });

    server.on("/wifi/data", HTTP_GET, [&server]() {
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
        if (!server.hasArg("ssid")) {
            server.send(400, "text/plain", "SSID tidak ada");
            return;
        }

        String ssid = server.arg("ssid");
        String pass = server.arg("pass");

        bool useStatic = server.hasArg("static");

        IPAddress ip, gw, sn;
        if (useStatic) {
            ip.fromString(server.arg("ip"));
            gw.fromString(server.arg("gw"));
            sn.fromString(server.arg("sn"));
        }

        addOrUpdateWiFi(ssid, pass, useStatic, ip, gw, sn);

        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/wifi/delete", HTTP_POST, [&server]() {
        if (!server.hasArg("ssid")) {
            server.send(
                400, "application/json",
                "{\"status\":\"error\",\"message\":\"SSID tidak ada\"}");
            return;
        }

        if (deleteWiFi(server.arg("ssid")))
            server.send(200, "application/json", "{\"status\":\"ok\"}");
        else
            server.send(
                404, "application/json",
                "{\"status\":\"error\",\"message\":\"SSID tidak ditemukan\"}");
    });

    server.on("/wifi/test", HTTP_POST, [&server]() {
        if (!server.hasArg("ssid")) {
            server.send(400, "application/json",
                        "{\"error\":\"ssid required\"}");
            return;
        }

        String ssid = server.arg("ssid");
        String pass = server.arg("pass");

        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());

        unsigned long start = millis();

        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            delay(300);
        }

        if (WiFi.status() != WL_CONNECTED) {
            server.send(500, "application/json", "{\"ok\":false}");
            WiFi.disconnect(false);
            return;
        }

        IPAddress ip = WiFi.localIP();
        IPAddress gw = WiFi.gatewayIP();
        IPAddress sn = WiFi.subnetMask();

        String json = "{";
        json += "\"ok\":true,";
        json += "\"ip\":\"" + ip.toString() + "\",";
        json += "\"gw\":\"" + gw.toString() + "\",";
        json += "\"sn\":\"" + sn.toString() + "\"";
        json += "}";

        WiFi.disconnect(false); // STA putus, AP tetap hidup

        server.send(200, "application/json", json);
    });
}