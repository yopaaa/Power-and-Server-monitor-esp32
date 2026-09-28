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
static unsigned long lastAutoCycleTime = 0;
static unsigned long lastWifiRetry = 0;
static int retryWifiIndex = 0;

// Button state tracking (Tap to wake, Long Press to cycle page)
static bool buttonWasPressed = false;
static unsigned long buttonPressStart = 0;
static bool longPressTriggered = false;
static bool wokeUpOnThisPress = false;
static const unsigned long LONG_PRESS_DURATION = 1000; // 1.0 detik untuk ganti halaman

void setup()
{
    Serial.begin(115200);

    // Konfigurasi pin input sensor sentuh & tombol
    pinMode(TTP223_PIN, INPUT_PULLDOWN); // TTP223: HIGH saat disentuh
    pinMode(BOOT_PIN, INPUT_PULLUP);     // Tombol BOOT: LOW saat ditekan
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    lcdInit();                           // Inisialisasi layar LCD & tampilkan splash boot
    updateBootProgress(10, "Booting System...", "Tahan Tombol utk Hotspot AP");

    // ── Cek apakah tombol ditekan saat booting untuk masuk Mode Hotspot AP ──
    bool enterAPConfig = false;
    unsigned long bootCheckStart = millis();
    while (millis() - bootCheckStart < 1500) {
        if (digitalRead(TTP223_PIN) == HIGH || digitalRead(BOOT_PIN) == LOW) {
            enterAPConfig = true;
            break;
        }
        delay(20);
    }

    if (enterAPConfig) {
        Serial.println("[BOOT] Button held at boot -> Entering Hotspot AP Config Mode");
        updateBootProgress(35, "AP Mode Selected", "Starting ESP32-Config...");
        initDeviceID();
        initPZEM();
        startAP();
        setupWeb();
        initUDPDiscovery();
        // Tetap di layar AP Mode sampai user mengatur WiFi atau me-reboot
        return;
    }

    // ── Boot Normal (Hotspot AP TIDAK AKTIF) ──
    Serial.println("[BOOT] Normal Boot (Hotspot AP OFF)");
    updateBootProgress(20, "Starting Normal Mode", "WiFi Client Mode");

    initServerMonitor();                 // Inisialisasi daftar server (Beszel Multi-Server)
    initBeszelClient();                  // Muat konfigurasi Beszel Hub dari LittleFS
    initPowerLogger();                   // Muat konfigurasi Power Logger dari LittleFS
    initDeviceID();
    updateBootProgress(35, "Initializing Sensors...", "PZEM-004T v3.0");
    initPZEM();

    loadWiFiList();
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    bool wifiOk = connectSavedWiFi();
    if (wifiOk) {
        if (getBeszelConfig().isConfigured) {
            updateBootProgress(95, "Syncing Beszel...", "Fetching server stats");
            String syncMsg;
            syncBeszelSystems(syncMsg);
        }
        updateBootProgress(100, "WiFi Connected!", WiFi.localIP().toString());
    } else {
        // Gagal / tidak ada wifi: JANGAN nyalakan AP! Tetap offline normal
        Serial.println("[BOOT] WiFi not connected. Running in Offline Mode (Hotspot OFF)");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        updateBootProgress(100, "Offline Mode", "Running locally (No WiFi)");
    }
    delay(400);

    setupWeb();
    initUDPDiscovery();

    // Render tampilan awal sesuai halaman aktif (default: Power Meter)
    if (getDisplayPage() == PAGE_SERVER_MONITOR) {
        drawServerMonitorFrame(getCurrentServer(), getCurrentServerIndex(), getServerCount());
    } else {
        drawPowerMeterFrame();
    }
    lcdResetActivity(); // Mulai timer 30 detik untuk LCD sleep
}

void loop()
{
    server.handleClient();
    handleWebReboot();
    handleUDPDiscovery();

    unsigned long currentMillis = millis();

    // ── 1. Background Auto-Reconnect WiFi (Jika Offline dan Router Menyala Kembali) ──
    if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED && wifiCount > 0) {
        if (currentMillis - lastWifiRetry >= 20000) { // Coba tiap 20 detik
            lastWifiRetry = currentMillis;
            if (retryWifiIndex >= wifiCount) retryWifiIndex = 0;
            Serial.printf("[WiFi] Background reconnect to: %s\n", wifiList[retryWifiIndex].ssid.c_str());
            WiFi.disconnect();
            WiFi.begin(wifiList[retryWifiIndex].ssid.c_str(), wifiList[retryWifiIndex].pass.c_str());
            retryWifiIndex++;
        }
    }

    // ── 2. Logika Tombol: Tap Bangunkan Layar vs Tekan Lama Ganti Tampilan ──
    bool isButtonPressed = (digitalRead(TTP223_PIN) == HIGH) || (digitalRead(BOOT_PIN) == LOW);

    // Edge: Tombol baru saja ditekan / disentuh
    if (isButtonPressed && !buttonWasPressed) {
        buttonWasPressed = true;
        buttonPressStart = currentMillis;
        longPressTriggered = false;

        if (lcdIsSleeping()) {
            // Layar sedang MATI: Cukup bangunkan layar dan redraw frame!
            lcdWake();
            wokeUpOnThisPress = true; // Tandai agar tekanan ini TIDAK memicu pergantian halaman
            Serial.println("[Button] Tap detected -> Waking screen from sleep");
        } else {
            // Layar sedang HIDUP: Reset timer 30 detik
            wokeUpOnThisPress = false;
            lcdResetActivity();
        }
    }

    // Tombol sedang DITAHAN (Holding)
    if (isButtonPressed && buttonWasPressed) {
        // Hanya ganti tampilan jika layar sudah hidup sebelum ditekan, dan belum di-trigger
        if (!wokeUpOnThisPress && !longPressTriggered && !lcdIsSleeping()) {
            if (currentMillis - buttonPressStart >= LONG_PRESS_DURATION) {
                longPressTriggered = true;
                cycleNextScreen();
                lastAutoCycleTime = currentMillis;
                lcdResetActivity();
                Serial.println("[Button] Long press (1s) -> Switch Carousel Page");
            }
        }
    }

    // Edge: Tombol baru saja dilepas
    if (!isButtonPressed && buttonWasPressed) {
        buttonWasPressed = false;
        wokeUpOnThisPress = false;
        longPressTriggered = false;
        lcdResetActivity();
    }

    // ── 3. Pengatur Layar LCD: Auto-Sleep & Auto-Wake Berkala ──
    handleLcdTimeout();

    // ── 4. Rotasi Otomatis (Auto-Cycle tiap interval jika diaktifkan dan layar sedang menyala) ──
    if (!lcdIsSleeping() && isAutoCycleEnabled() && (currentMillis - lastAutoCycleTime >= getAutoCycleInterval())) {
        lastAutoCycleTime = currentMillis;
        cycleNextScreen();
    }

    // ── 5. Loop Server Monitor (Beszel) ──
    if (currentMillis - lastServerUpdate >= 1000) {
        lastServerUpdate = currentMillis;

        if (!pollBeszelMetrics()) {
            updateServerMonitorMock();
        }

        // Hanya refresh rendering LCD jika layar sedang menyala
        if (!lcdIsSleeping() && getDisplayPage() == PAGE_SERVER_MONITOR) {
            updateServerMonitorDisplay(getCurrentServer(), getCurrentServerIndex(), getServerCount());
        }
    }

    // ── 6. Loop Power Meter (PZEM-004T) ──
    if (currentMillis - lastPzemRead >= 1000) {
        lastPzemRead = currentMillis;
        readPZEM(); // Sensor PZEM tetap selalu dibaca di background

        // Hanya refresh rendering LCD jika layar sedang menyala
        if (!lcdIsSleeping() && getDisplayPage() == PAGE_POWER_METER) {
            String statusInfo = "";
            if (WiFi.status() == WL_CONNECTED) {
                statusInfo = WiFi.localIP().toString();
            } else if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
                statusInfo = "AP: " + WiFi.softAPIP().toString();
            } else {
                statusInfo = "No WiFi (Offline)";
            }
            updatePowerMeterDisplay(getPZEMMetrics(), statusInfo);
        }
    }

    // ── 7. Loop Power Logger (Kirim Data ke Server Golang) ──
    // Tetap berjalan mengirim data di background walau LCD sedang mati
    handlePowerLoggerLoop(getPZEMMetrics());
}