#include "./SavedWifi.h"
#include "./Web.h"
#include "LCD.h"
#include <ArduinoJson.h>

#include <ESPmDNS.h>
#include <time.h>

WiFiCred wifiList[MAX_WIFI];
uint8_t wifiCount = 0;

Preferences prefs;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600; // WIB
const int daylightOffset_sec = 0;

void initNTP()
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    Serial.println("Waiting NTP...");
    lcdPrintCenterX("Waiting NTP...", 2, TFT_BLUE, 2);

    unsigned long start = millis();
    time_t now = time(nullptr);

    while (now < 100000 && millis() - start < 10000) {
        delay(200);
        now = time(nullptr);
    }

    if (now < 100000) {
        Serial.println("NTP Failed");
    } else {
        Serial.println("NTP Ready");
    }
}

void saveWiFiList()
{
   prefs.begin("wifi_store", false);
    prefs.putUChar("count", wifiCount);
    
    for (int i = 0; i < wifiCount; i++) {
        String idx = String(i);
        prefs.putString(("s" + idx).c_str(), wifiList[i].ssid);
        prefs.putString(("p" + idx).c_str(), wifiList[i].pass);
        prefs.putBool(("st" + idx).c_str(), wifiList[i].useStatic);
        prefs.putUInt(("ip" + idx).c_str(), (uint32_t)wifiList[i].ip);
        prefs.putUInt(("gw" + idx).c_str(), (uint32_t)wifiList[i].gw);
        prefs.putUInt(("sn" + idx).c_str(), (uint32_t)wifiList[i].sn);
    }
    prefs.end();
    Serial.println("WiFi list saved to Preferences");
}

void loadWiFiList()
{
   prefs.begin("wifi_store", true); // Read-only mode
    wifiCount = prefs.getUChar("count", 0);
    
    if (wifiCount > MAX_WIFI) wifiCount = MAX_WIFI;

    for (int i = 0; i < wifiCount; i++) {
        String idx = String(i);
        wifiList[i].ssid = prefs.getString(("s" + idx).c_str(), "");
        wifiList[i].pass = prefs.getString(("p" + idx).c_str(), "");
        wifiList[i].useStatic = prefs.getBool(("st" + idx).c_str(), false);
        wifiList[i].ip = IPAddress(prefs.getUInt(("ip" + idx).c_str(), 0));
        wifiList[i].gw = IPAddress(prefs.getUInt(("gw" + idx).c_str(), 0));
        wifiList[i].sn = IPAddress(prefs.getUInt(("sn" + idx).c_str(), 0));
    }
    prefs.end();
    Serial.println("WiFi list loaded from Preferences");
}

int scanWiFi(String found[], int max)
{
    Serial.println("Scan WiFi...");
    int n = WiFi.scanNetworks();
    if (n <= 0) return 0;

    int count = 0;
    for (int i = 0; i < n && count < max; i++) {
        found[count++] = WiFi.SSID(i);
        
            bool open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
        
        
        Serial.printf("%d. %s | RSSI: %d dBm | %s\n", count, WiFi.SSID(i).c_str(), WiFi.RSSI(i), open ? "OPEN" : "SECURE");
    }
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
                    IPAddress dns(8,8,8,8);
                    WiFi.config(wifiList[i].ip, wifiList[i].gw, wifiList[i].sn, dns);
                }

                WiFi.begin(wifiList[i].ssid.c_str(), wifiList[i].pass.c_str());

                unsigned long start = millis();
                while (millis() - start < 8000) {
                    if (WiFi.status() == WL_CONNECTED) {
                        Serial.println("--------------------------");
                        Serial.println("Connected to: " + wifiList[i].ssid);
                        lcdPrintCenterX("Connected to:", 0, TFT_BLUE, 2);
                        lcdPrintCenterX(wifiList[i].ssid, 1, TFT_BLUE, 2);
                        Serial.print("RSSI       : ");
                        Serial.print("Current IP  : ");
                        Serial.println(
                            WiFi.localIP()); // Menampilkan IP Address
                        Serial.print("Gateway     : ");
                        Serial.println(WiFi.gatewayIP());
                        Serial.println("--------------------------");
                        initNTP();
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

    Serial.println("AP started. IP: " + WiFi.softAPIP().toString());

    if (MDNS.begin("esp")) {
        Serial.println("mDNS: http://esp.local");
        MDNS.addService("http", "tcp", 80);
    }
    showCenterImage("/qr-wifi.jpg");
}