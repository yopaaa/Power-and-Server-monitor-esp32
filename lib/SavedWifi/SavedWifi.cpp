#include "./SavedWifi.h"
#include "./Web.h"
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>

WiFiCred wifiList[MAX_WIFI];
uint8_t wifiCount = 0;

void saveWiFiList()
{
    File file = LittleFS.open("/wifi.json", "w");
    if (!file) {
        Serial.println("Gagal buka file untuk write");
        return;
    }

    String json = "{";
    json += "\"count\":" + String(wifiCount) + ",";
    json += "\"list\":[";

    for (int i = 0; i < wifiCount; i++) {
        json += "{";
        json += "\"ssid\":\"" + wifiList[i].ssid + "\",";
        json += "\"pass\":\"" + wifiList[i].pass + "\",";
        json += "\"useStatic\":" +
                String(wifiList[i].useStatic ? "true" : "false") + ",";
        json += "\"ip\":\"" + wifiList[i].ip.toString() + "\",";
        json += "\"gw\":\"" + wifiList[i].gw.toString() + "\",";
        json += "\"sn\":\"" + wifiList[i].sn.toString() + "\"";
        json += "}";

        if (i < wifiCount - 1)
            json += ",";
    }

    json += "]}";

    file.print(json);
    file.close();

    Serial.println("WiFi list saved to LittleFS");
}

void loadWiFiList()
{
    if (!LittleFS.exists("/wifi.json")) {
        Serial.println("wifi.json tidak ada");
        return;
    }

    File file = LittleFS.open("/wifi.json", "r");
    if (!file) {
        Serial.println("Gagal buka wifi.json");
        return;
    }

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("JSON parse error");
        return;
    }

    wifiCount = doc["count"];
    if (wifiCount > MAX_WIFI)
        wifiCount = MAX_WIFI;

    for (int i = 0; i < wifiCount; i++) {
        wifiList[i].ssid = doc["list"][i]["ssid"].as<String>();
        wifiList[i].pass = doc["list"][i]["pass"].as<String>();
        wifiList[i].useStatic = doc["list"][i]["useStatic"];

        wifiList[i].ip.fromString(doc["list"][i]["ip"].as<String>());
        wifiList[i].gw.fromString(doc["list"][i]["gw"].as<String>());
        wifiList[i].sn.fromString(doc["list"][i]["sn"].as<String>());
    }

    Serial.println("WiFi list loaded from LittleFS");
}

int scanWiFi(String found[], int max)
{
    Serial.println("Scan WiFi...");

    int n = WiFi.scanNetworks();
    int count = 0;

    if (n <= 0) {
        Serial.println("Tidak ada WiFi ditemukan");
        return 0;
    }

    Serial.print("Ditemukan ");
    Serial.print(n);
    Serial.println(" jaringan");

    for (int i = 0; i < n && count < max; i++) {
        String ssid = WiFi.SSID(i);

        found[count++] = ssid;

        Serial.print(count);
        Serial.print(". ");
        Serial.print(ssid);
        Serial.print(" | RSSI: ");
        Serial.print(WiFi.RSSI(i));
        Serial.print(" dBm");
        Serial.print(" | Enkripsi: ");
        Serial.println(WiFi.encryptionType(i) == ENC_TYPE_NONE ? "OPEN"
                                                               : "SECURE");
    }

    Serial.println("Scan selesai");
    return count;
}

bool connectSavedWiFi()
{
    String found[10];
    int foundCount = scanWiFi(found, 10);

    for (int i = 0; i < wifiCount; i++) {
        for (int j = 0; j < foundCount; j++) {
            if (wifiList[i].ssid == found[j]) {

                WiFi.mode(WIFI_STA);

                if (wifiList[i].useStatic) {
                    WiFi.config(wifiList[i].ip, wifiList[i].gw, wifiList[i].sn);
                }

                WiFi.begin(wifiList[i].ssid.c_str(), wifiList[i].pass.c_str());

                unsigned long start = millis();
                while (millis() - start < 8000) {
                    if (WiFi.status() == WL_CONNECTED) {
                        Serial.println("--------------------------");
                        Serial.println("Connected to: " + wifiList[i].ssid);
                        Serial.print("Current IP  : ");
                        Serial.println(
                            WiFi.localIP()); // Menampilkan IP Address
                        Serial.print("Gateway     : ");
                        Serial.println(WiFi.gatewayIP());
                        Serial.println("--------------------------");
                        return true;
                    }
                    delay(500);
                }
            }
        }
    }

    return false;
}

void addOrUpdateWiFi(String ssid, String pass, bool useStatic, IPAddress ip,
                     IPAddress gw, IPAddress sn)
{
    for (int i = 0; i < wifiCount; i++) {
        if (wifiList[i].ssid == ssid) {
            wifiList[i].pass = pass;
            wifiList[i].useStatic = useStatic;
            wifiList[i].ip = ip;
            wifiList[i].gw = gw;
            wifiList[i].sn = sn;
            saveWiFiList();
            return;
        }
    }

    if (wifiCount < MAX_WIFI) {
        wifiList[wifiCount].ssid = ssid;
        wifiList[wifiCount].pass = pass;
        wifiList[wifiCount].useStatic = useStatic;
        wifiList[wifiCount].ip = ip;
        wifiList[wifiCount].gw = gw;
        wifiList[wifiCount].sn = sn;
        wifiCount++;
    } else {
        wifiList[0].ssid = ssid;
        wifiList[0].pass = pass;
        wifiList[0].useStatic = useStatic;
        wifiList[0].ip = ip;
        wifiList[0].gw = gw;
        wifiList[0].sn = sn;
    }

    saveWiFiList();
}

bool deleteWiFi(String ssid)
{
    for (int i = 0; i < wifiCount; i++) {
        if (wifiList[i].ssid == ssid) {

            // Geser data setelahnya ke kiri
            for (int j = i; j < wifiCount - 1; j++) {
                wifiList[j] = wifiList[j + 1];
            }

            wifiCount--;

            saveWiFiList();
            return true;
        }
    }

    return false; // SSID tidak ditemukan
}

void startAP()
{
    Serial.println("Starting AP mode");

    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-Config");

    Serial.println("AP started");
    Serial.println("AP IP: " + WiFi.softAPIP().toString());

    // Inisialisasi mDNS
    if (MDNS.begin("esp")) { // Akan bisa diakses via http://esp.local
        Serial.println("mDNS responder started: http://esp.local");
    }

    // Tambahkan service HTTP agar mDNS lebih mudah ditemukan oleh browser
    MDNS.addService("http", "tcp", 80);
}