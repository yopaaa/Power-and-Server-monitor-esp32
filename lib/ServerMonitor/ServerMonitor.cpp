#include "ServerMonitor.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <math.h>

static std::vector<ServerMetrics> servers;
static int activeIndex = 0;
static const char* SERVERS_FILE = "/servers.json";

static void initDefaultMockServers()
{
    servers.clear();

    // ── Server 1: Primary Homelab / Proxmox ──
    ServerMetrics s1;
    s1.name = "PROXMOX-NODE";
    s1.host = "192.168.1.50";
    s1.systemId = "mock-proxmox";
    s1.isOnline = true;
    s1.cpuPercent = 24.5f;
    s1.cpuTemp = 47.0f;
    s1.ramPercent = 58.2f;
    s1.ramUsedGB = 9.3f;
    s1.ramTotalGB = 16.0f;
    s1.diskPercent = 42.0f;
    s1.diskUsedGB = 210.0f;
    s1.diskTotalGB = 500.0f;
    s1.uptime = "14d 6h";
    servers.push_back(s1);

    // ── Server 2: TrueNAS Storage ──
    ServerMetrics s2;
    s2.name = "NAS-STORAGE";
    s2.host = "192.168.1.55";
    s2.systemId = "mock-nas";
    s2.isOnline = true;
    s2.cpuPercent = 12.0f;
    s2.cpuTemp = 41.5f;
    s2.ramPercent = 38.0f;
    s2.ramUsedGB = 3.0f;
    s2.ramTotalGB = 8.0f;
    s2.diskPercent = 78.5f;
    s2.diskUsedGB = 3140.0f;
    s2.diskTotalGB = 4000.0f;
    s2.uptime = "45d 18h";
    servers.push_back(s2);

    // ── Server 3: Docker Swarm / Microservices ──
    ServerMetrics s3;
    s3.name = "DOCKER-NODE";
    s3.host = "192.168.1.60";
    s3.systemId = "mock-docker";
    s3.isOnline = true;
    s3.cpuPercent = 65.4f;
    s3.cpuTemp = 58.0f;
    s3.ramPercent = 74.5f;
    s3.ramUsedGB = 11.9f;
    s3.ramTotalGB = 16.0f;
    s3.diskPercent = 55.0f;
    s3.diskUsedGB = 275.0f;
    s3.diskTotalGB = 500.0f;
    s3.uptime = "3d 11h";
    servers.push_back(s3);

    activeIndex = 0;
}

void initServerMonitor()
{
    LittleFS.begin(true);

    if (!loadServersFromFS() || servers.empty()) {
        Serial.println("[ServerMonitor] Belum ada konfigurasi di LittleFS, memuat server mock default...");
        initDefaultMockServers();
        saveServersToFS();
    } else {
        Serial.printf("[ServerMonitor] Berhasil memuat %d server dari LittleFS\n", (int)servers.size());
    }
    activeIndex = 0;
}

bool saveServersToFS()
{
    File f = LittleFS.open(SERVERS_FILE, "w");
    if (!f) {
        Serial.println("[ServerMonitor] Gagal membuka /servers.json untuk ditulis");
        return false;
    }

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (const auto &s : servers) {
        JsonObject obj = arr.add<JsonObject>();
        obj["name"] = s.name;
        obj["host"] = s.host;
        obj["systemId"] = s.systemId;
        obj["isOnline"] = s.isOnline;
        obj["cpuPercent"] = s.cpuPercent;
        obj["cpuTemp"] = s.cpuTemp;
        obj["ramPercent"] = s.ramPercent;
        obj["ramUsedGB"] = s.ramUsedGB;
        obj["ramTotalGB"] = s.ramTotalGB;
        obj["diskPercent"] = s.diskPercent;
        obj["diskUsedGB"] = s.diskUsedGB;
        obj["diskTotalGB"] = s.diskTotalGB;
        obj["uptime"] = s.uptime;
    }

    serializeJson(doc, f);
    f.close();
    return true;
}

bool loadServersFromFS()
{
    if (!LittleFS.exists(SERVERS_FILE)) return false;

    File f = LittleFS.open(SERVERS_FILE, "r");
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        Serial.printf("[ServerMonitor] JSON parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    if (arr.isNull() || arr.size() == 0) return false;

    servers.clear();
    for (JsonObject obj : arr) {
        ServerMetrics s;
        s.name = obj["name"] | "SERVER";
        s.host = obj["host"] | "0.0.0.0";
        s.systemId = obj["systemId"] | "";
        s.isOnline = obj["isOnline"] | false;
        s.cpuPercent = obj["cpuPercent"] | 0.0f;
        s.cpuTemp = obj["cpuTemp"] | 0.0f;
        s.ramPercent = obj["ramPercent"] | 0.0f;
        s.ramUsedGB = obj["ramUsedGB"] | 0.0f;
        s.ramTotalGB = obj["ramTotalGB"] | 16.0f;
        s.diskPercent = obj["diskPercent"] | 0.0f;
        s.diskUsedGB = obj["diskUsedGB"] | 0.0f;
        s.diskTotalGB = obj["diskTotalGB"] | 500.0f;
        s.uptime = obj["uptime"] | "-";
        servers.push_back(s);
    }

    return !servers.empty();
}

void updateServerMonitorMock()
{
    if (servers.empty()) return;

    for (size_t i = 0; i < servers.size(); i++) {
        // Hanya update mock jika tidak terhubung ke Beszel asli atau berawalan mock
        if (servers[i].systemId.startsWith("mock") || servers[i].systemId.isEmpty()) {
            float deltaCpu = ((float)(random(-15, 16)) / 10.0f);
            servers[i].cpuPercent += deltaCpu;
            if (servers[i].cpuPercent < 5.0f) servers[i].cpuPercent = 5.0f;
            if (servers[i].cpuPercent > 98.0f) servers[i].cpuPercent = 98.0f;

            servers[i].cpuTemp = 40.0f + (servers[i].cpuPercent * 0.25f);

            float deltaRam = ((float)(random(-5, 6)) / 10.0f);
            servers[i].ramPercent += deltaRam;
            if (servers[i].ramPercent < 10.0f) servers[i].ramPercent = 10.0f;
            if (servers[i].ramPercent > 95.0f) servers[i].ramPercent = 95.0f;
            servers[i].ramUsedGB = (servers[i].ramPercent / 100.0f) * servers[i].ramTotalGB;
        }
    }
}

int getServerCount()
{
    return (int)servers.size();
}

int getCurrentServerIndex()
{
    return activeIndex;
}

const ServerMetrics& getCurrentServer()
{
    static ServerMetrics emptySrv = {"NO SERVER", "0.0.0.0", "", false, 0, 0, 0, 0, 0, 0, 0, 0, "-"};
    if (servers.empty() || activeIndex < 0 || activeIndex >= (int)servers.size()) {
        return emptySrv;
    }
    return servers[activeIndex];
}

const ServerMetrics* getServerAt(int index)
{
    if (index >= 0 && index < (int)servers.size()) {
        return &servers[index];
    }
    return nullptr;
}

void nextServer()
{
    if (servers.empty()) return;
    activeIndex = (activeIndex + 1) % (int)servers.size();
}

void prevServer()
{
    if (servers.empty()) return;
    activeIndex = (activeIndex - 1 + (int)servers.size()) % (int)servers.size();
}

void selectServer(int index)
{
    if (index >= 0 && index < (int)servers.size()) {
        activeIndex = index;
    }
}

void setServerMetrics(int index, const ServerMetrics &metrics)
{
    if (index >= 0 && index < (int)servers.size()) {
        servers[index] = metrics;
    }
}

int addServer(const String &name, const String &host, const String &systemId)
{
    ServerMetrics s;
    s.name = name;
    s.host = host;
    s.systemId = systemId;
    s.isOnline = true;
    s.cpuPercent = 10.0f;
    s.cpuTemp = 42.0f;
    s.ramPercent = 25.0f;
    s.ramUsedGB = 4.0f;
    s.ramTotalGB = 16.0f;
    s.diskPercent = 30.0f;
    s.diskUsedGB = 150.0f;
    s.diskTotalGB = 500.0f;
    s.uptime = "0d 1h";
    servers.push_back(s);
    saveServersToFS();
    return (int)servers.size() - 1;
}

bool deleteServer(int index)
{
    if (index >= 0 && index < (int)servers.size()) {
        servers.erase(servers.begin() + index);
        if (activeIndex >= (int)servers.size()) {
            activeIndex = servers.empty() ? 0 : (int)servers.size() - 1;
        }
        saveServersToFS();
        return true;
    }
    return false;
}

void clearServers()
{
    servers.clear();
    activeIndex = 0;
    saveServersToFS();
}

void addOrUpdateServer(const ServerMetrics &s)
{
    // Cari apakah sudah ada berdasarkan systemId atau name
    for (size_t i = 0; i < servers.size(); i++) {
        if ((!s.systemId.isEmpty() && servers[i].systemId == s.systemId) ||
            (s.systemId.isEmpty() && servers[i].name == s.name)) {
            servers[i] = s;
            return;
        }
    }
    // Jika baru, tambahkan
    servers.push_back(s);
    saveServersToFS();
}
