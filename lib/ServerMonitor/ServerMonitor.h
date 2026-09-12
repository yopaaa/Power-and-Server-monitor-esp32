#ifndef SERVER_MONITOR_H
#define SERVER_MONITOR_H

#include <Arduino.h>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Server Metrics Data Structure (Beszel System Record)
// ─────────────────────────────────────────────────────────────────────────────
struct ServerMetrics {
    String name;         // Nama / Title server (contoh: "HOMELAB-01")
    String host;         // IP atau domain (contoh: "192.168.1.50")
    bool   isOnline;     // Status koneksi (Online / Offline)
    float  cpuPercent;   // Persentase penggunaan CPU (0 - 100%)
    float  cpuTemp;      // Suhu CPU (°C)
    float  ramPercent;   // Persentase penggunaan RAM (0 - 100%)
    float  ramUsedGB;    // RAM terpakai dalam GB
    float  ramTotalGB;   // Total kapasitas RAM dalam GB
    float  diskPercent;  // Persentase penggunaan Disk (0 - 100%)
    float  diskUsedGB;   // Disk terpakai dalam GB
    float  diskTotalGB;  // Total kapasitas Disk dalam GB
    String uptime;       // Durasi server aktif (contoh: "14d 6h")
};

void initServerMonitor();
void updateServerMonitorMock(); // Simulasi fluktuasi data hidup untuk pengujian UI

int getServerCount();
int getCurrentServerIndex();
const ServerMetrics& getCurrentServer();
void nextServer();
void prevServer();
void selectServer(int index);

void setServerMetrics(int index, const ServerMetrics &metrics);
int addServer(const String &name, const String &host);

#endif
