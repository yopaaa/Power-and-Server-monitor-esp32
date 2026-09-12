#include "ServerMonitor.h"
#include <math.h>

static std::vector<ServerMetrics> servers;
static int activeIndex = 0;

void initServerMonitor()
{
    servers.clear();

    // ── Server 1: Primary Homelab / Proxmox ──
    ServerMetrics s1;
    s1.name = "PROXMOX-NODE";
    s1.host = "192.168.1.50";
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

void updateServerMonitorMock()
{
    if (servers.empty()) return;

    // Simulasi data realistis (fluktuasi halus agar layar terasa hidup)
    for (size_t i = 0; i < servers.size(); i++) {
        // CPU berubah acak ±1.5%
        float deltaCpu = ((float)(random(-15, 16)) / 10.0f);
        servers[i].cpuPercent += deltaCpu;
        if (servers[i].cpuPercent < 5.0f) servers[i].cpuPercent = 5.0f;
        if (servers[i].cpuPercent > 98.0f) servers[i].cpuPercent = 98.0f;

        // Suhu CPU mengikuti beban CPU sedikit
        servers[i].cpuTemp = 40.0f + (servers[i].cpuPercent * 0.25f);

        // RAM fluktuasi kecil ±0.2%
        float deltaRam = ((float)(random(-5, 6)) / 10.0f);
        servers[i].ramPercent += deltaRam;
        if (servers[i].ramPercent < 10.0f) servers[i].ramPercent = 10.0f;
        if (servers[i].ramPercent > 95.0f) servers[i].ramPercent = 95.0f;
        servers[i].ramUsedGB = (servers[i].ramPercent / 100.0f) * servers[i].ramTotalGB;
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
    static ServerMetrics emptySrv = {"NO SERVER", "0.0.0.0", false, 0, 0, 0, 0, 0, 0, 0, 0, "-"};
    if (servers.empty() || activeIndex < 0 || activeIndex >= (int)servers.size()) {
        return emptySrv;
    }
    return servers[activeIndex];
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

int addServer(const String &name, const String &host)
{
    ServerMetrics s;
    s.name = name;
    s.host = host;
    s.isOnline = false;
    s.cpuPercent = 0.0f;
    s.cpuTemp = 0.0f;
    s.ramPercent = 0.0f;
    s.ramUsedGB = 0.0f;
    s.ramTotalGB = 16.0f;
    s.diskPercent = 0.0f;
    s.diskUsedGB = 0.0f;
    s.diskTotalGB = 500.0f;
    s.uptime = "-";
    servers.push_back(s);
    return (int)servers.size() - 1;
}
