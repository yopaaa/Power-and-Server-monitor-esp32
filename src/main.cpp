#include "SavedWifi.h"
#include "Web.h"
#include <ESPmDNS.h>

#include "LCD.h"
#include "Lcd_api.h"
#include "Devices.h"
#include "PZEMManager.h"
#include "ServerMonitor.h"

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

void setup()
{
    Serial.begin(115200);

    // Konfigurasi pin input sensor sentuh & tombol
    pinMode(TTP223_PIN, INPUT_PULLDOWN); // TTP223 default: LOW saat diam, HIGH saat disentuh
    pinMode(BOOT_PIN, INPUT_PULLUP);     // Tombol BOOT: HIGH saat diam, LOW saat ditekan
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    initServerMonitor(); // Inisialisasi daftar server (Beszel Multi-Server)
    lcdInit();           // Membuka LCD dengan default PAGE_SERVER_MONITOR
    initDeviceID();
    initPZEM();

    loadWiFiList();
    WiFi.mode(WIFI_STA);

    if (!connectSavedWiFi()) {
        startAP();
    }

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
        cycleNextScreen();
    }

    unsigned long currentMillis = millis();

    // ── 2. Loop Server Monitor (Beszel) ──
    if (currentMillis - lastServerUpdate >= 1000) {
        lastServerUpdate = currentMillis;
        updateServerMonitorMock(); // Simulasi fluktuasi halus untuk testing visual

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
}