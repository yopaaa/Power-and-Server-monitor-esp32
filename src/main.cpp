#include "SavedWifi.h"
#include "Web.h"
#include <ESPmDNS.h>

#include "LCD.h"
#include "Lcd_api.h"
#include "Devices.h"
#include "PZEMManager.h"
#include "ServerMonitor.h"
#include "BeszelClient.h"
#include "PowerLogger.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Pin Configuration untuk Tombol / Touch Sensor
// ─────────────────────────────────────────────────────────────────────────────
#ifndef TTP223_PIN
#define TTP223_PIN 4   // Sensor Sentuh TTP223 (Active-HIGH, Hubungkan ke GPIO 4)
#endif

#ifndef BOOT_PIN
#define BOOT_PIN 0     // Tombol BOOT ESP32 (Active-LOW, Backup pengujian)
#endif

static unsigned long lastPzemRead = 0;
static unsigned long lastServerUpdate = 0;
static int lastTouchState = LOW;
static int lastBootState = HIGH;
static unsigned long lastTriggerTime = 0;
static unsigned long lastAutoCycleTime = 0;

void setup()
{
    Serial.begin(115200);

    // Konfigurasi pin input sensor sentuh & tombol
    pinMode(TTP223_PIN, INPUT_PULLDOWN); // TTP223 default: LOW saat diam, HIGH saat disentuh
    pinMode(BOOT_PIN, INPUT_PULLUP);     // Tombol BOOT: HIGH saat diam, LOW saat ditekan
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    lcdInit();                           // Inisialisasi layar LCD & tampilkan loading screen awal
    updateBootProgress(15, "Starting System...", "ESP32 Dev Module");

    initServerMonitor();                 // Inisialisasi daftar server (Beszel Multi-Server)
    initBeszelClient();                  // Muat konfigurasi Beszel Hub dari LittleFS
    initPowerLogger();                   // Muat konfigurasi Power Logger dari LittleFS
    initDeviceID();
    updateBootProgress(30, "Initializing Sensors...", "PZEM-004T v3.0");
    initPZEM();

    loadWiFiList();
    WiFi.mode(WIFI_STA);

    bool wifiOk = connectSavedWiFi();
    if (!wifiOk) {
        startAP();
        setupWeb();
        initUDPDiscovery();
        // Tetap di layar AP Mode sampai user mengatur WiFi atau menekan tombol navigasi
        return;
    }

    // Jika WiFi terhubung dan Beszel terkonfigurasi, sinkronkan metrik awal
    if (getBeszelConfig().isConfigured) {
        updateBootProgress(95, "Syncing Beszel...", "Fetching server stats");
        String syncMsg;
        syncBeszelSystems(syncMsg);
    }

    updateBootProgress(100, "System Ready!", "Launching Dashboard");
    delay(400);

    setupWeb();
    initUDPDiscovery();

    // Render tampilan awal sesuai halaman aktif (default: Server 1)
    if (getDisplayPage() == PAGE_SERVER_MONITOR) {
        drawServerMonitorFrame(getCurrentServer(), getCurrentServerIndex(), getServerCount());
    } else {
        drawPowerMeterFrame();
    }
}

void loop()
{
    server.handleClient();
    handleWebReboot();
    handleUDPDiscovery();

    // ── 1. Deteksi Touch Sensor TTP223 & Tombol BOOT ──
    int touchState = digitalRead(TTP223_PIN);
    int bootState = digitalRead(BOOT_PIN);
    bool triggered = false;

    // TTP223: Terdeteksi sentuhan (Rising Edge: LOW -> HIGH)
    if (touchState == HIGH && lastTouchState == LOW) {
        triggered = true;
    }
    lastTouchState = touchState;

    // Tombol BOOT onboard: Tertekan (Falling Edge: HIGH -> LOW)
    if (bootState == LOW && lastBootState == HIGH) {
        triggered = true;
    }
    lastBootState = bootState;

    // Eksekusi perpindahan carousel (Debounce 250ms)
    // Urutan Loop: Power Meter -> Server 1 -> Server 2 -> Server 3 -> Power Meter...
    if (triggered && (millis() - lastTriggerTime > 250)) {
        lastTriggerTime = millis();
        lastAutoCycleTime = millis(); // Reset timer rotasi otomatis saat ditekan manual
        cycleNextScreen();
    }

    unsigned long currentMillis = millis();

    // ── Rotasi Otomatis (Auto-Cycle tiap 20 detik jika mode aktif) ──
    if (isAutoCycleEnabled() && (currentMillis - lastAutoCycleTime >= getAutoCycleInterval())) {
        lastAutoCycleTime = currentMillis;
        cycleNextScreen();
    }

    // ── 2. Loop Server Monitor (Beszel) ──
    if (currentMillis - lastServerUpdate >= 1000) {
        lastServerUpdate = currentMillis;

        // Coba polling data asli dari Beszel Hub; jika tidak terkonfigurasi jalankan mock halus
        if (!pollBeszelMetrics()) {
            updateServerMonitorMock();
        }

        if (getDisplayPage() == PAGE_SERVER_MONITOR) {
            updateServerMonitorDisplay(getCurrentServer(), getCurrentServerIndex(), getServerCount());
        }
    }

    // ── 3. Loop Power Meter (PZEM-004T) ──
    if (currentMillis - lastPzemRead >= 1000) {
        lastPzemRead = currentMillis;
        readPZEM();

        if (getDisplayPage() == PAGE_POWER_METER) {
            String statusInfo = "";
            if (WiFi.status() == WL_CONNECTED) {
                statusInfo = WiFi.localIP().toString();
            } else if (WiFi.getMode() == WIFI_AP) {
                statusInfo = "AP: " + WiFi.softAPIP().toString();
            } else {
                statusInfo = "No WiFi";
            }
            updatePowerMeterDisplay(getPZEMMetrics(), statusInfo);
        }
    }

    // ── 4. Loop Power Logger (Kirim Data ke Server Golang) ──
    handlePowerLoggerLoop(getPZEMMetrics());
}