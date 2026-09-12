#include "BeszelClient.h"
#include "ServerMonitor.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <vector>

static BeszelConfig config = {"", "", "", "", false, 5000};
static const char* BESZEL_CONFIG_FILE = "/beszel_config.json";
static unsigned long lastPollTime = 0;

void initBeszelClient()
{
    loadBeszelConfigFromFS();
}

const BeszelConfig& getBeszelConfig()
{
    return config;
}

void setBeszelConfig(const String &url, const String &email, const String &password)
{
    config.hubUrl = url;
    config.hubUrl.trim();
    while (config.hubUrl.endsWith("/")) {
        config.hubUrl.remove(config.hubUrl.length() - 1);
    }
    config.email = email;
    config.email.trim();
    config.password = password;
    config.isConfigured = (!config.hubUrl.isEmpty() && !config.email.isEmpty() && !config.password.isEmpty());
    config.token = ""; // Reset token agar login ulang
    saveBeszelConfigToFS();
}

bool saveBeszelConfigToFS()
{
    File f = LittleFS.open(BESZEL_CONFIG_FILE, "w");
    if (!f) return false;

    JsonDocument doc;
    doc["hubUrl"] = config.hubUrl;
    doc["email"] = config.email;
    doc["password"] = config.password;
    doc["pollIntervalMs"] = config.pollIntervalMs;
    doc["isConfigured"] = config.isConfigured;

    serializeJson(doc, f);
    f.close();
    return true;
}

bool loadBeszelConfigFromFS()
{
    if (!LittleFS.exists(BESZEL_CONFIG_FILE)) return false;

    File f = LittleFS.open(BESZEL_CONFIG_FILE, "r");
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) return false;

    config.hubUrl = doc["hubUrl"] | "";
    config.email = doc["email"] | "";
    config.password = doc["password"] | "";
    config.pollIntervalMs = doc["pollIntervalMs"] | 5000;
    config.isConfigured = doc["isConfigured"] | false;
    config.token = "";

    return config.isConfigured;
}

static bool performAuth(const String &url, const String &email, const String &password, String &tokenOut, String &errOut)
{
    if (WiFi.status() != WL_CONNECTED) {
        errOut = "ESP32 belum terhubung ke WiFi";
        return false;
    }

    HTTPClient http;
    http.setTimeout(3500);

    String authUrl = url + "/api/collections/users/auth-with-password";
    http.begin(authUrl);
    http.addHeader("Content-Type", "application/json");

    JsonDocument reqDoc;
    reqDoc["identity"] = email;
    reqDoc["password"] = password;
    String reqBody;
    serializeJson(reqDoc, reqBody);

    int httpCode = http.POST(reqBody);
    String resp = http.getString();
    http.end();

    // Jika gagal 404 (mungkin Beszel/PocketBase versi Admin), coba endpoint admin
    if (httpCode == 404) {
        http.begin(url + "/api/admins/auth-with-password");
        http.addHeader("Content-Type", "application/json");
        httpCode = http.POST(reqBody);
        resp = http.getString();
        http.end();
    }

    if (httpCode == 200) {
        JsonDocument resDoc;
        DeserializationError err = deserializeJson(resDoc, resp);
        if (!err && !resDoc["token"].isNull()) {
            tokenOut = resDoc["token"].as<String>();
            return true;
        } else {
            errOut = "Token tidak ditemukan dalam respons Hub";
            return false;
        }
    } else {
        errOut = "Gagal login (HTTP " + String(httpCode) + "). Periksa URL atau Email/Password.";
        return false;
    }
}

bool testBeszelHub(const String &url, const String &email, const String &password, String &outMsg, int &systemsFound)
{
    systemsFound = 0;
    String cleanUrl = url;
    cleanUrl.trim();
    while (cleanUrl.endsWith("/")) cleanUrl.remove(cleanUrl.length() - 1);

    String token;
    String authErr;
    if (!performAuth(cleanUrl, email, password, token, authErr)) {
        outMsg = authErr;
        return false;
    }

    // Ambil daftar sistem
    HTTPClient http;
    http.setTimeout(3500);
    http.begin(cleanUrl + "/api/collections/systems/records?perPage=50");
    http.addHeader("Authorization", token);
    http.addHeader("Accept", "application/json");

    int code = http.GET();
    String body = http.getString();
    http.end();

    if (code == 200) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (!err) {
            JsonArray items = doc["items"].as<JsonArray>();
            systemsFound = items.size();
            outMsg = "Koneksi Berhasil! Terotentikasi dan mendeteksi " + String(systemsFound) + " server di Beszel Hub.";
            return true;
        }
    }

    outMsg = "Login berhasil, namun gagal mengambil sistem (HTTP " + String(code) + ")";
    return false;
}

bool authenticateBeszel(String &err)
{
    if (!config.isConfigured) {
        err = "Beszel Hub belum dikonfigurasi";
        return false;
    }
    return performAuth(config.hubUrl, config.email, config.password, config.token, err);
}

static void parseSystemItem(JsonObject item, ServerMetrics &s)
{
    s.systemId = item["id"] | "";
    s.name = item["name"] | "SERVER";
    s.host = item["host"] | "0.0.0.0";
    String st = item["status"] | "down";
    s.isOnline = (st == "up");

    // Ekstraksi info metrik
    JsonDocument infoDoc;
    bool hasParsedInfo = false;

    if (item["info"].is<JsonObject>()) {
        infoDoc.set(item["info"]);
        hasParsedInfo = true;
    } else if (item["info"].is<const char*>()) {
        String infoStr = item["info"].as<const char*>();
        if (!deserializeJson(infoDoc, infoStr)) {
            hasParsedInfo = true;
        }
    }

    if (hasParsedInfo) {
        s.cpuPercent = infoDoc["cpu"] | infoDoc["cp"] | 0.0f;

        // Suhu CPU: dt adalah Dashboard Temperature di Beszel info struct
        if (infoDoc["dt"].is<float>()) {
            s.cpuTemp = infoDoc["dt"].as<float>();
        } else if (infoDoc["temp"].is<float>()) {
            s.cpuTemp = infoDoc["temp"].as<float>();
        }

        // RAM percent: "mp" (MemPct) di Beszel
        if (infoDoc["mp"].is<float>()) {
            s.ramPercent = infoDoc["mp"].as<float>();
        } else if (infoDoc["ram"].is<float>()) {
            s.ramPercent = infoDoc["ram"].as<float>();
        }

        // Disk percent: "dp" (DiskPct) di Beszel
        if (infoDoc["dp"].is<float>()) {
            s.diskPercent = infoDoc["dp"].as<float>();
        } else if (infoDoc["disk"].is<float>()) {
            s.diskPercent = infoDoc["disk"].as<float>();
        }

        // Metrik RAM langsung jika ada di info
        if (infoDoc["m"].is<float>()) {
            s.ramTotalGB = infoDoc["m"].as<float>();
        } else if (infoDoc["m_total"].is<float>()) {
            s.ramTotalGB = infoDoc["m_total"].as<float>();
        }

        if (infoDoc["mu"].is<float>()) {
            s.ramUsedGB = infoDoc["mu"].as<float>();
        } else if (infoDoc["m_used"].is<float>()) {
            s.ramUsedGB = infoDoc["m_used"].as<float>();
        } else if (s.ramTotalGB > 0.0f && s.ramPercent > 0.0f) {
            s.ramUsedGB = (s.ramPercent / 100.0f) * s.ramTotalGB;
        }

        // Metrik Disk langsung jika ada di info
        if (infoDoc["d"].is<float>()) {
            s.diskTotalGB = infoDoc["d"].as<float>();
        } else if (infoDoc["d_total"].is<float>()) {
            s.diskTotalGB = infoDoc["d_total"].as<float>();
        }

        if (infoDoc["du"].is<float>()) {
            s.diskUsedGB = infoDoc["du"].as<float>();
        } else if (infoDoc["d_used"].is<float>()) {
            s.diskUsedGB = infoDoc["d_used"].as<float>();
        } else if (s.diskTotalGB > 0.0f && s.diskPercent > 0.0f) {
            s.diskUsedGB = (s.diskPercent / 100.0f) * s.diskTotalGB;
        }

        if (!infoDoc["u"].isNull()) {
            long upSec = infoDoc["u"] | 0;
            if (upSec > 0) {
                long days = upSec / 86400;
                long hours = (upSec % 86400) / 3600;
                s.uptime = String(days) + "d " + String(hours) + "h";
            }
        }
    }
}

static void fetchSystemStats(const String &token)
{
    if (token.isEmpty() || getServerCount() == 0) return;

    HTTPClient http;
    http.setTimeout(3500);
    http.begin(config.hubUrl + "/api/collections/system_stats/records?perPage=50&sort=-created&fields=system,stats");
    http.addHeader("Authorization", token);
    http.addHeader("Accept", "application/json");

    int code = http.GET();
    if (code == 200) {
        String body = http.getString();
        http.end();

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (!err) {
            JsonArray items = doc["items"].as<JsonArray>();
            std::vector<String> processedSystems;

            for (JsonObject item : items) {
                String sysId = item["system"] | "";
                if (sysId.isEmpty()) continue;

                bool alreadyProcessed = false;
                for (const auto &id : processedSystems) {
                    if (id == sysId) {
                        alreadyProcessed = true;
                        break;
                    }
                }
                if (alreadyProcessed) continue;

                JsonObject stats = item["stats"];
                if (stats.isNull()) continue;

                for (int i = 0; i < getServerCount(); i++) {
                    const ServerMetrics* sm = getServerAt(i);
                    if (sm && sm->systemId == sysId) {
                        ServerMetrics updated = *sm;

                        if (stats["m"].is<float>()) {
                            updated.ramTotalGB = stats["m"].as<float>();
                        }
                        if (stats["mu"].is<float>()) {
                            updated.ramUsedGB = stats["mu"].as<float>();
                        }
                        if (stats["mp"].is<float>()) {
                            updated.ramPercent = stats["mp"].as<float>();
                        }
                        if (updated.ramUsedGB <= 0.0f && updated.ramTotalGB > 0.0f && updated.ramPercent > 0.0f) {
                            updated.ramUsedGB = (updated.ramPercent / 100.0f) * updated.ramTotalGB;
                        }

                        if (stats["d"].is<float>()) {
                            updated.diskTotalGB = stats["d"].as<float>();
                        }
                        if (stats["du"].is<float>()) {
                            updated.diskUsedGB = stats["du"].as<float>();
                        }
                        if (stats["dp"].is<float>()) {
                            updated.diskPercent = stats["dp"].as<float>();
                        }
                        if (updated.diskUsedGB <= 0.0f && updated.diskTotalGB > 0.0f && updated.diskPercent > 0.0f) {
                            updated.diskUsedGB = (updated.diskPercent / 100.0f) * updated.diskTotalGB;
                        }

                        if (stats["cpu"].is<float>()) {
                            updated.cpuPercent = stats["cpu"].as<float>();
                        }

                        if (!stats["t"].isNull()) {
                            if (stats["t"].is<JsonObject>()) {
                                float maxT = 0.0f;
                                for (JsonPair kv : stats["t"].as<JsonObject>()) {
                                    float val = kv.value().as<float>();
                                    if (val > maxT) maxT = val;
                                }
                                if (maxT > 0.0f) updated.cpuTemp = maxT;
                            } else if (stats["t"].is<float>()) {
                                float val = stats["t"].as<float>();
                                if (val > 0.0f) updated.cpuTemp = val;
                            }
                        }

                        setServerMetrics(i, updated);
                        processedSystems.push_back(sysId);
                        break;
                    }
                }
            }
        }
    } else {
        http.end();
    }

    // Fallback: periksa system_details jika ada server dengan ramTotalGB <= 0.0f
    bool needDetails = false;
    for (int i = 0; i < getServerCount(); i++) {
        const ServerMetrics* sm = getServerAt(i);
        if (sm && sm->ramTotalGB <= 0.0f) {
            needDetails = true;
            break;
        }
    }

    if (needDetails) {
        http.setTimeout(3500);
        http.begin(config.hubUrl + "/api/collections/system_details/records?perPage=50&fields=id,memory");
        http.addHeader("Authorization", token);
        http.addHeader("Accept", "application/json");

        code = http.GET();
        if (code == 200) {
            String body = http.getString();
            http.end();

            JsonDocument doc;
            if (!deserializeJson(doc, body)) {
                JsonArray items = doc["items"].as<JsonArray>();
                for (JsonObject item : items) {
                    String sysId = item["id"] | "";
                    uint64_t memBytes = item["memory"] | 0;
                    if (sysId.isEmpty() || memBytes == 0) continue;

                    for (int i = 0; i < getServerCount(); i++) {
                        const ServerMetrics* sm = getServerAt(i);
                        if (sm && sm->systemId == sysId && sm->ramTotalGB <= 0.0f) {
                            ServerMetrics updated = *sm;
                            updated.ramTotalGB = (float)memBytes / (1024.0f * 1024.0f * 1024.0f);
                            if (updated.ramUsedGB <= 0.0f && updated.ramPercent > 0.0f) {
                                updated.ramUsedGB = (updated.ramPercent / 100.0f) * updated.ramTotalGB;
                            }
                            setServerMetrics(i, updated);
                            break;
                        }
                    }
                }
            }
        } else {
            http.end();
        }
    }
}

bool syncBeszelSystems(String &outMsg)
{
    if (WiFi.status() != WL_CONNECTED) {
        outMsg = "WiFi belum terhubung";
        return false;
    }

    if (config.token.isEmpty()) {
        String authErr;
        if (!authenticateBeszel(authErr)) {
            outMsg = authErr;
            return false;
        }
    }

    HTTPClient http;
    http.setTimeout(3500);
    http.begin(config.hubUrl + "/api/collections/systems/records?perPage=50");
    http.addHeader("Authorization", config.token);
    http.addHeader("Accept", "application/json");

    int code = http.GET();
    String body = http.getString();
    http.end();

    if (code == 401 || code == 403) {
        // Token kadaluarsa, coba login ulang sekali lagi
        config.token = "";
        String authErr;
        if (authenticateBeszel(authErr)) {
            http.begin(config.hubUrl + "/api/collections/systems/records?perPage=50");
            http.addHeader("Authorization", config.token);
            code = http.GET();
            body = http.getString();
            http.end();
        }
    }

    if (code == 200) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            outMsg = "Gagal memproses JSON respons Beszel";
            return false;
        }

        JsonArray items = doc["items"].as<JsonArray>();
        if (items.size() == 0) {
            outMsg = "Berhasil terhubung, namun tidak ada server terdaftar di Beszel Hub";
            return true;
        }

        // Hapus server mock lama jika ini sinkronisasi pertama kali
        bool hasMockOnly = true;
        for (int i = 0; i < getServerCount(); i++) {
            const ServerMetrics* s = getServerAt(i);
            if (s && !s->systemId.startsWith("mock") && !s->systemId.isEmpty()) {
                hasMockOnly = false;
                break;
            }
        }
        if (hasMockOnly) {
            clearServers();
        }

        int count = 0;
        for (JsonObject item : items) {
            ServerMetrics s;
            parseSystemItem(item, s);
            addOrUpdateServer(s);
            count++;
        }

        // Ambil metrik detail (RAM total/used, Disk total/used, Suhu) dari system_stats / system_details
        fetchSystemStats(config.token);

        saveServersToFS();
        outMsg = "Berhasil menyinkronkan " + String(count) + " server dari Beszel Hub!";
        return true;
    }

    outMsg = "Gagal menghubungi Beszel Hub (HTTP " + String(code) + ")";
    return false;
}

bool pollBeszelMetrics()
{
    if (!config.isConfigured || WiFi.status() != WL_CONNECTED) return false;

    unsigned long now = millis();
    if (now - lastPollTime < config.pollIntervalMs) return false;
    lastPollTime = now;

    if (config.token.isEmpty()) {
        String authErr;
        if (!authenticateBeszel(authErr)) return false;
    }

    HTTPClient http;
    http.setTimeout(2500); // Ringan non-blocking
    http.begin(config.hubUrl + "/api/collections/systems/records?perPage=50");
    http.addHeader("Authorization", config.token);
    http.addHeader("Accept", "application/json");

    int code = http.GET();
    if (code == 200) {
        String body = http.getString();
        http.end();

        JsonDocument doc;
        if (!deserializeJson(doc, body)) {
            JsonArray items = doc["items"].as<JsonArray>();
            for (JsonObject item : items) {
                String sysId = item["id"] | "";
                for (int i = 0; i < getServerCount(); i++) {
                    const ServerMetrics* s = getServerAt(i);
                    if (s && s->systemId == sysId) {
                        ServerMetrics updated = *s;
                        parseSystemItem(item, updated);
                        setServerMetrics(i, updated);
                        break;
                    }
                }
            }
            fetchSystemStats(config.token);
            return true;
        }
    } else {
        http.end();
        if (code == 401 || code == 403) {
            config.token = ""; // Reset token agar request berikutnya login ulang
        }
    }

    return false;
}

bool testServerConnection(const String &host, String &outMsg)
{
    if (WiFi.status() != WL_CONNECTED) {
        outMsg = "WiFi tidak terhubung";
        return false;
    }

    String cleanHost = host;
    cleanHost.trim();
    if (cleanHost.startsWith("http://")) cleanHost = cleanHost.substring(7);
    if (cleanHost.startsWith("https://")) cleanHost = cleanHost.substring(8);
    int slashIdx = cleanHost.indexOf('/');
    if (slashIdx > 0) cleanHost = cleanHost.substring(0, slashIdx);
    int portIdx = cleanHost.indexOf(':');
    int port = 80;
    if (portIdx > 0) {
        port = cleanHost.substring(portIdx + 1).toInt();
        cleanHost = cleanHost.substring(0, portIdx);
    }

    unsigned long start = millis();
    WiFiClient client;
    client.setTimeout(2); // 2 detik timeout
    if (client.connect(cleanHost.c_str(), port)) {
        unsigned long lat = millis() - start;
        client.stop();
        outMsg = "Online! Terhubung ke " + cleanHost + ":" + String(port) + " (" + String(lat) + "ms)";
        return true;
    } else {
        // Coba port 45876 (Beszel Agent default) atau port 22 (SSH)
        if (client.connect(cleanHost.c_str(), 45876)) {
            unsigned long lat = millis() - start;
            client.stop();
            outMsg = "Online! Beszel Agent terdeteksi di port 45876 (" + String(lat) + "ms)";
            return true;
        }
        outMsg = "Offline / Timeout menghubungkan ke " + cleanHost;
        return false;
    }
}
