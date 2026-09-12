#ifndef BESZEL_CLIENT_H
#define BESZEL_CLIENT_H

#include <Arduino.h>

struct BeszelConfig {
    String hubUrl;        // Contoh: "http://192.168.1.100:8090"
    String email;         // Akun PocketBase / Beszel
    String password;
    String token;         // JWT auth token aktif
    bool   isConfigured;
    unsigned long pollIntervalMs; // Default 5000ms
};

void initBeszelClient();
const BeszelConfig& getBeszelConfig();
void setBeszelConfig(const String &url, const String &email, const String &password);
bool saveBeszelConfigToFS();
bool loadBeszelConfigFromFS();

// Tes koneksi & otentikasi ke Beszel Hub tanpa menyimpan
bool testBeszelHub(const String &url, const String &email, const String &password, String &outMsg, int &systemsFound);

// Tes ping / koneksi ke host server individual
bool testServerConnection(const String &host, String &outMsg);

// Otentikasi dan dapatkan token baru
bool authenticateBeszel(String &err);

// Sinkronisasi sistem yang ada di Beszel Hub ke daftar server ESP32
bool syncBeszelSystems(String &outMsg);

// Polling metrik terbaru secara non-blocking
bool pollBeszelMetrics();

#endif
