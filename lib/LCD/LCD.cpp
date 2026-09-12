#include "LCD.h"
#include <math.h>
#include <Preferences.h>

TFT_eSPI tft = TFT_eSPI();
const int ledChannel = 0;
const int freq = 5000;
const int resolution = 8; // 8-bit (0-255)

// Flicker-free display cache
static float prevPower = -999.0f;
static float prevVolt = -999.0f;
static float prevCurr = -999.0f;
static float prevEnergy = -999.0f;
static float prevFreq = -999.0f;
static float prevPf = -999.0f;
static bool  prevConn = false;
static bool  prevInit = false;
static String prevStatus = "";

static bool isScreenInverted = false;
static bool isAutoCycle = false;
static uint32_t autoCycleIntervalMs = 20000; // 20 detik
static DisplayPage currentPage = PAGE_SERVER_MONITOR; // Default to Server Monitor as requested
static int screenIndex = 1; // 0 = Power Meter, 1..N = Server 1..N

void loadLcdSettings() {
    Preferences prefs;
    if (prefs.begin("lcd_cfg", true)) {
        isAutoCycle = prefs.getBool("autocycle", false);
        autoCycleIntervalMs = prefs.getUInt("cycle_int", 20000);
        prefs.end();
    }
}

void saveLcdSettings() {
    Preferences prefs;
    if (prefs.begin("lcd_cfg", false)) {
        prefs.putBool("autocycle", isAutoCycle);
        prefs.putUInt("cycle_int", autoCycleIntervalMs);
        prefs.end();
    }
}

void setAutoCycle(bool enabled, uint32_t intervalMs) {
    isAutoCycle = enabled;
    if (intervalMs >= 5000) {
        autoCycleIntervalMs = intervalMs;
    }
    saveLcdSettings();
}

bool isAutoCycleEnabled() {
    return isAutoCycle;
}

uint32_t getAutoCycleInterval() {
    return autoCycleIntervalMs;
}

void setDisplayPage(DisplayPage page) {
    currentPage = page;
    if (currentPage == PAGE_SERVER_MONITOR) {
        drawServerMonitorFrame(getCurrentServer(), getCurrentServerIndex(), getServerCount());
    } else {
        drawPowerMeterFrame();
    }
}

DisplayPage getDisplayPage() {
    return currentPage;
}

void cycleDisplayPage() {
    if (currentPage == PAGE_SERVER_MONITOR) {
        setDisplayPage(PAGE_POWER_METER);
    } else {
        setDisplayPage(PAGE_SERVER_MONITOR);
    }
}

int getScreenIndex() {
    return screenIndex;
}

void setScreenIndex(int idx) {
    int totalScreens = 1 + getServerCount();
    if (totalScreens <= 0) return;
    screenIndex = (idx % totalScreens + totalScreens) % totalScreens;

    if (screenIndex == 0) {
        setDisplayPage(PAGE_POWER_METER);
    } else {
        selectServer(screenIndex - 1);
        setDisplayPage(PAGE_SERVER_MONITOR);
    }
}

void cycleNextScreen() {
    setScreenIndex(screenIndex + 1);
}

void cyclePrevScreen() {
    setScreenIndex(screenIndex - 1);
}

void lcdBacklight(bool on) { 
    ledcWrite(ledChannel, on ? 240 : 0);
}

void lcdBacklight(int brightness) {
    if (brightness < 0) brightness = 0;
    if (brightness > 255) brightness = 255;
    ledcWrite(ledChannel, brightness);
}

void lcdToggleInversion() {
    isScreenInverted = !isScreenInverted;
    tft.invertDisplay(isScreenInverted);
    if (currentPage == PAGE_SERVER_MONITOR) {
        drawServerMonitorFrame(getCurrentServer(), getCurrentServerIndex(), getServerCount());
    } else {
        drawPowerMeterFrame();
    }
}

bool lcdIsInverted() {
    return isScreenInverted;
}

void lcdInit()
{
    loadLcdSettings();

    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(TFT_BL, ledChannel);

    lcdBacklight(0);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(C_BG);

    lcdBacklight(240); // 94% PWM: deep blacks, minimal backlight bleed
    if (currentPage == PAGE_SERVER_MONITOR) {
        drawServerMonitorFrame(getCurrentServer(), getCurrentServerIndex(), getServerCount());
    } else {
        drawPowerMeterFrame();
    }
}

void lcdClear(uint16_t color) { tft.fillScreen(color); }

void lcdPrint(String text, int x, int y, uint16_t color, uint8_t size)
{
    tft.setTextColor(color);
    tft.setTextSize(size);
    tft.setCursor(x, y);
    tft.print(text);
}

void lcdPrintCenterX(String text, int line, uint16_t color, uint8_t size)
{
    int lineHeight = (size * 8) + 4; 
    int y = (line * lineHeight) + 2;

    tft.setTextSize(size);
    int textWidth = tft.textWidth(text);
    int x = (tft.width() - textWidth) / 2;
    if (x < 0) x = 0;

    tft.setTextColor(color);
    tft.setCursor(x, y);
    tft.print(text);
}

// ─────────────────────────────────────────────────────────────────────────────
// Power Meter Layout & Display
// ─────────────────────────────────────────────────────────────────────────────
void drawPowerMeterFrame()
{
    prevInit = false;
    tft.fillScreen(C_BG);

    // ── Header (Y: 0..24) ──
    tft.fillRect(0, 0, 240, 3, C_ACCENT); // Top accent stripe
    tft.setTextSize(2);
    tft.setTextColor(C_TEXT, C_BG);
    tft.drawString("POWER METER", 12, 6);

    tft.drawFastHLine(0, 24, 240, C_BORDER);

    // ── Hero Card: Active Power (W) (Y: 28..92, H: 64) ──
    tft.fillRect(8, 28, 224, 64, C_CARD);
    tft.drawRect(8, 28, 224, 64, C_BORDER);
    tft.fillRect(8, 28, 4, 64, C_ACCENT); // Left accent pill
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT, C_CARD);
    tft.drawString("ACTIVE POWER", 18, 33);
    tft.setTextSize(2);
    tft.setTextColor(C_ACCENT_HI, C_CARD);
    tft.drawString("W", 195, 58);

    // ── Middle Row: Voltage & Current (Y: 96..152, H: 56) ──
    // Left: Voltage (V)
    tft.fillRect(8, 96, 108, 56, C_CARD);
    tft.drawRect(8, 96, 108, 56, C_BORDER);
    tft.fillRect(8, 96, 3, 56, C_PRIMARY);
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT, C_CARD);
    tft.drawString("VOLTAGE", 16, 101);
    tft.setTextSize(2);
    tft.setTextColor(C_ACCENT_HI, C_CARD);
    tft.drawString("V", 98, 124);

    // Right: Current (A)
    tft.fillRect(124, 96, 108, 56, C_CARD);
    tft.drawRect(124, 96, 108, 56, C_BORDER);
    tft.fillRect(124, 96, 3, 56, C_PRIMARY);
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT, C_CARD);
    tft.drawString("CURRENT", 132, 101);
    tft.setTextSize(2);
    tft.setTextColor(C_ACCENT_HI, C_CARD);
    tft.drawString("A", 214, 124);

    // ── Bottom Row: Energy & Freq/PF (Y: 156..212, H: 56) ──
    // Left: Energy (kWh)
    tft.fillRect(8, 156, 108, 56, C_CARD);
    tft.drawRect(8, 156, 108, 56, C_BORDER);
    tft.fillRect(8, 156, 3, 56, C_PRIMARY);
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT, C_CARD);
    tft.drawString("ENERGY", 16, 161);
    tft.setTextSize(1);
    tft.setTextColor(C_ACCENT_HI, C_CARD);
    tft.drawString("kWh", 88, 192);

    // Right: Frequency & Power Factor
    tft.fillRect(124, 156, 108, 56, C_CARD);
    tft.drawRect(124, 156, 108, 56, C_BORDER);
    tft.fillRect(124, 156, 3, 56, C_PRIMARY);
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT, C_CARD);
    tft.drawString("GRID STATUS", 132, 161);

    // ── Footer Bar (Y: 216..239) ──
    tft.drawFastHLine(0, 216, 240, C_BORDER);
}

void updatePowerMeterDisplay(const PZEMMetrics &m, const String &statusInfo)
{
    char buf[20];
    bool connChanged = (!prevInit || prevConn != m.isConnected);

    // 1. Power (W)
    if (!prevInit || connChanged || (m.isConnected && fabs(m.power - prevPower) >= 0.1f)) {
        prevPower = m.power;
        tft.setTextColor(m.isConnected ? C_VALUE : C_DIM, C_CARD);
        tft.setTextPadding(168); // Atomically clears & draws in 1 pass, no flicker!
        if (!m.isConnected) {
            tft.drawString("--.-", 20, 50, 4);
        } else if (m.power >= 1000.0f) {
            snprintf(buf, sizeof(buf), "%.2f", m.power / 1000.0f);
            tft.drawString(buf, 20, 50, 4);
            tft.setTextPadding(28);
            tft.setTextColor(C_ACCENT_HI, C_CARD);
            tft.drawString("kW", 188, 58, 2);
        } else {
            snprintf(buf, sizeof(buf), "%.1f", m.power);
            tft.drawString(buf, 20, 50, 4);
            tft.setTextPadding(28);
            tft.setTextColor(C_ACCENT_HI, C_CARD);
            tft.drawString("W ", 195, 58, 2);
        }
    }

    // 2. Voltage (V)
    if (!prevInit || connChanged || (m.isConnected && fabs(m.voltage - prevVolt) >= 0.1f)) {
        prevVolt = m.voltage;
        tft.setTextColor(m.isConnected ? C_VALUE : C_DIM, C_CARD);
        tft.setTextPadding(78);
        if (!m.isConnected) {
            tft.drawString("--.-", 16, 118, 4);
        } else {
            snprintf(buf, sizeof(buf), "%.1f", m.voltage);
            tft.drawString(buf, 16, 118, 4);
        }
    }

    // 3. Current (A)
    if (!prevInit || connChanged || (m.isConnected && fabs(m.current - prevCurr) >= 0.01f)) {
        prevCurr = m.current;
        tft.setTextColor(m.isConnected ? C_VALUE : C_DIM, C_CARD);
        tft.setTextPadding(78);
        if (!m.isConnected) {
            tft.drawString("--.--", 132, 118, 4);
        } else {
            snprintf(buf, sizeof(buf), "%.2f", m.current);
            tft.drawString(buf, 132, 118, 4);
        }
    }

    // 4. Energy (kWh)
    if (!prevInit || connChanged || (m.isConnected && fabs(m.energy - prevEnergy) >= 0.01f)) {
        prevEnergy = m.energy;
        tft.setTextColor(m.isConnected ? C_VALUE : C_DIM, C_CARD);
        tft.setTextPadding(70);
        if (!m.isConnected) {
            tft.drawString("--.-", 16, 178, 4);
        } else {
            if (m.energy < 100.0f) {
                snprintf(buf, sizeof(buf), "%.2f", m.energy);
            } else {
                snprintf(buf, sizeof(buf), "%.1f", m.energy);
            }
            tft.drawString(buf, 16, 178, 4);
        }
    }

    // 5. Grid Status: Frequency & PF
    if (!prevInit || connChanged || (m.isConnected && (fabs(m.frequency - prevFreq) >= 0.1f || fabs(m.pf - prevPf) >= 0.01f))) {
        prevFreq = m.frequency;
        prevPf = m.pf;
        tft.setTextPadding(94);
        if (!m.isConnected) {
            tft.setTextColor(C_DIM, C_CARD);
            tft.drawString("F: -- Hz", 132, 176, 2);
            tft.drawString("PF: ---", 132, 192, 2);
        } else {
            tft.setTextColor(C_TEXT, C_CARD);
            snprintf(buf, sizeof(buf), "F: %.1fHz", m.frequency);
            tft.drawString(buf, 132, 176, 2);

            tft.setTextColor(C_ACCENT_HI, C_CARD);
            snprintf(buf, sizeof(buf), "PF: %.2f", m.pf);
            tft.drawString(buf, 132, 192, 2);
        }
    }

    // 6. Footer Status Bar
    if (!prevInit || connChanged || statusInfo != prevStatus) {
        prevStatus = statusInfo;
        tft.setTextPadding(115);
        if (m.isConnected) {
            tft.setTextColor(C_ACCENT_HI, C_BG);
            tft.drawString("* PZEM OK", 10, 222, 2);
        } else {
            tft.setTextColor(C_ALERT, C_BG);
            tft.drawString("! NO COMM", 10, 222, 2);
        }

        tft.setTextColor(C_TEXT, C_BG);
        tft.setTextPadding(110);
        tft.drawRightString(statusInfo, 230, 224, 1);
    }

    prevConn = m.isConnected;
    prevInit = true;
    tft.setTextPadding(0); // Reset padding
}

// ─────────────────────────────────────────────────────────────────────────────
// Server Monitor Layout & Display (Beszel Multi-Server)
// ─────────────────────────────────────────────────────────────────────────────
static float srvPrevCpu = -999.0f;
static float srvPrevTemp = -999.0f;
static float srvPrevRam = -999.0f;
static float srvPrevRamUsed = -999.0f;
static float srvPrevDisk = -999.0f;
static float srvPrevDiskUsed = -999.0f;
static int   srvPrevIdx = -1;
static int   srvPrevCpuBar = -1;
static int   srvPrevRamBar = -1;
static int   srvPrevDiskBar = -1;
static bool  srvFrameDrawn = false;

void drawServerMonitorFrame(const ServerMetrics &srv, int serverIdx, int totalServers)
{
    srvPrevIdx = serverIdx;
    srvPrevCpu = -999.0f;
    srvPrevTemp = -999.0f;
    srvPrevRam = -999.0f;
    srvPrevRamUsed = -999.0f;
    srvPrevDisk = -999.0f;
    srvPrevDiskUsed = -999.0f;
    srvPrevCpuBar = -1;
    srvPrevRamBar = -1;
    srvPrevDiskBar = -1;
    srvFrameDrawn = true;

    tft.fillScreen(C_BG);

    // ── Header (Y: 0..26) ──
    tft.fillRect(0, 0, 240, 3, C_ACCENT); // Top accent stripe
    tft.setTextSize(2);
    tft.setTextColor(C_TEXT, C_BG);

    // Truncate name if too long for header
    String title = srv.name;
    if (title.length() > 10) title = title.substring(0, 10);
    tft.drawString(title, 12, 6);

    // Index badge e.g. [1/3]
    char idxBuf[16];
    snprintf(idxBuf, sizeof(idxBuf), "[%d/%d]", serverIdx + 1, totalServers > 0 ? totalServers : 1);
    tft.setTextSize(1);
    tft.setTextColor(C_ACCENT_HI, C_BG);
    tft.drawString(idxBuf, 168, 10);

    // Online Status Dot
    uint16_t dotColor = srv.isOnline ? C_ACCENT_HI : C_ALERT;
    tft.fillCircle(218, 13, 4, dotColor);

    tft.drawFastHLine(0, 26, 240, C_BORDER);

    // ── Card 1: CPU & Temperature (Y: 30..86, H: 56) ──
    tft.fillRect(8, 30, 224, 56, C_CARD);
    tft.drawRect(8, 30, 224, 56, C_BORDER);
    tft.fillRect(8, 30, 4, 56, C_ACCENT); // Left accent stripe
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT, C_CARD);
    tft.drawString("CPU LOAD", 18, 34);

    // Progress bar border (X: 18, Y: 72, W: 200, H: 6)
    tft.drawRect(18, 72, 200, 6, C_BORDER);

    // ── Card 2: Memory / RAM (Y: 92..148, H: 56) ──
    tft.fillRect(8, 92, 224, 56, C_CARD);
    tft.drawRect(8, 92, 224, 56, C_BORDER);
    tft.fillRect(8, 92, 4, 56, C_PRIMARY); // Left accent stripe
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT, C_CARD);
    tft.drawString("MEMORY (RAM)", 18, 96);

    // Progress bar border (X: 18, Y: 134, W: 200, H: 6)
    tft.drawRect(18, 134, 200, 6, C_BORDER);

    // ── Card 3: Storage / Disk (Y: 154..210, H: 56) ──
    tft.fillRect(8, 154, 224, 56, C_CARD);
    tft.drawRect(8, 154, 224, 56, C_BORDER);
    tft.fillRect(8, 154, 4, 56, C_PRIMARY); // Left accent stripe
    tft.setTextSize(1);
    tft.setTextColor(C_TEXT, C_CARD);
    tft.drawString("STORAGE (ROOT)", 18, 158);

    // Progress bar border (X: 18, Y: 196, W: 200, H: 6)
    tft.drawRect(18, 196, 200, 6, C_BORDER);

    // ── Footer Bar (Y: 216..240) ──
    tft.drawFastHLine(0, 216, 240, C_BORDER);
}

void updateServerMonitorDisplay(const ServerMetrics &srv, int serverIdx, int totalServers)
{
    if (!srvFrameDrawn || serverIdx != srvPrevIdx) {
        drawServerMonitorFrame(srv, serverIdx, totalServers);
    }

    char buf[32];

    // 1. CPU Update
    if (fabs(srv.cpuPercent - srvPrevCpu) >= 0.1f) {
        srvPrevCpu = srv.cpuPercent;
        tft.setTextColor(C_VALUE, C_CARD);
        tft.setTextPadding(105);
        snprintf(buf, sizeof(buf), "%.1f%%", srv.cpuPercent);
        tft.drawString(buf, 20, 48, 4);

        // Progress bar
        int fillW = (int)((srv.cpuPercent / 100.0f) * 196.0f);
        if (fillW < 0) fillW = 0;
        if (fillW > 196) fillW = 196;
        if (fillW != srvPrevCpuBar) {
            srvPrevCpuBar = fillW;
            uint16_t barColor = (srv.cpuPercent >= 85.0f) ? C_ALERT : ((srv.cpuPercent >= 70.0f) ? TFT_ORANGE : C_ACCENT_HI);
            if (fillW > 0) tft.fillRect(20, 74, fillW, 2, barColor);
            if (196 - fillW > 0) tft.fillRect(20 + fillW, 74, 196 - fillW, 2, C_CARD);
        }
    }

    // CPU Temp
    if (fabs(srv.cpuTemp - srvPrevTemp) >= 0.5f) {
        srvPrevTemp = srv.cpuTemp;
        tft.setTextPadding(65);
        tft.setTextColor(srv.cpuTemp >= 75.0f ? C_ALERT : C_ACCENT_HI, C_CARD);
        snprintf(buf, sizeof(buf), "%.0f'C", srv.cpuTemp);
        tft.drawRightString(buf, 222, 34, 1);
    }

    // 2. RAM Update
    if (fabs(srv.ramPercent - srvPrevRam) >= 0.1f || fabs(srv.ramUsedGB - srvPrevRamUsed) >= 0.05f) {
        srvPrevRam = srv.ramPercent;
        srvPrevRamUsed = srv.ramUsedGB;
        tft.setTextColor(C_VALUE, C_CARD);
        tft.setTextPadding(105);
        snprintf(buf, sizeof(buf), "%.1f%%", srv.ramPercent);
        tft.drawString(buf, 20, 110, 4);

        // Subtitle: Used / Total GB
        tft.setTextPadding(100);
        tft.setTextColor(C_TEXT, C_CARD);
        if (fmod(srv.ramTotalGB, 1.0f) < 0.05f) {
            snprintf(buf, sizeof(buf), "%.1f/%.0fGB", srv.ramUsedGB, srv.ramTotalGB);
        } else {
            snprintf(buf, sizeof(buf), "%.1f/%.1fGB", srv.ramUsedGB, srv.ramTotalGB);
        }
        tft.drawRightString(buf, 222, 96, 1);

        // Progress bar
        int fillW = (int)((srv.ramPercent / 100.0f) * 196.0f);
        if (fillW < 0) fillW = 0;
        if (fillW > 196) fillW = 196;
        if (fillW != srvPrevRamBar) {
            srvPrevRamBar = fillW;
            uint16_t barColor = (srv.ramPercent >= 90.0f) ? C_ALERT : C_ACCENT_HI;
            if (fillW > 0) tft.fillRect(20, 136, fillW, 2, barColor);
            if (196 - fillW > 0) tft.fillRect(20 + fillW, 136, 196 - fillW, 2, C_CARD);
        }
    }

    // 3. Disk Update
    if (fabs(srv.diskPercent - srvPrevDisk) >= 0.1f || fabs(srv.diskUsedGB - srvPrevDiskUsed) >= 0.5f) {
        srvPrevDisk = srv.diskPercent;
        srvPrevDiskUsed = srv.diskUsedGB;
        tft.setTextColor(C_VALUE, C_CARD);
        tft.setTextPadding(105);
        snprintf(buf, sizeof(buf), "%.1f%%", srv.diskPercent);
        tft.drawString(buf, 20, 172, 4);

        // Subtitle: Used / Total
        tft.setTextPadding(100);
        tft.setTextColor(C_TEXT, C_CARD);
        if (srv.diskTotalGB >= 1000.0f) {
            snprintf(buf, sizeof(buf), "%.1f/%.1fTB", srv.diskUsedGB / 1000.0f, srv.diskTotalGB / 1000.0f);
        } else {
            snprintf(buf, sizeof(buf), "%.0f/%.0fGB", srv.diskUsedGB, srv.diskTotalGB);
        }
        tft.drawRightString(buf, 222, 158, 1);

        // Progress bar
        int fillW = (int)((srv.diskPercent / 100.0f) * 196.0f);
        if (fillW < 0) fillW = 0;
        if (fillW > 196) fillW = 196;
        if (fillW != srvPrevDiskBar) {
            srvPrevDiskBar = fillW;
            uint16_t barColor = (srv.diskPercent >= 90.0f) ? C_ALERT : C_ACCENT_HI;
            if (fillW > 0) tft.fillRect(20, 198, fillW, 2, barColor);
            if (196 - fillW > 0) tft.fillRect(20 + fillW, 198, 196 - fillW, 2, C_CARD);
        }
    }

    // 4. Footer Bar: Uptime & Host
    tft.setTextPadding(110);
    tft.setTextColor(C_ACCENT_HI, C_BG);
    snprintf(buf, sizeof(buf), "UP: %s", srv.uptime.c_str());
    tft.drawString(buf, 10, 224, 1);

    tft.setTextColor(C_TEXT, C_BG);
    tft.setTextPadding(110);
    tft.drawRightString(srv.host, 230, 224, 1);

    tft.setTextPadding(0);
}

// ─────────────────────────────────────────────────────────────────────────────
// OTA Update Screens
// ─────────────────────────────────────────────────────────────────────────────
void drawOtaStart() {
    int w = tft.width();

    tft.fillScreen(C_BG);
    tft.fillRect(0, 0, w, 2, C_ACCENT);

    // Header
    tft.fillRect(0, 2, w, 36, C_BG);
    tft.drawFastHLine(0, 38, w, C_BORDER);
    tft.setTextColor(C_TEXT);
    tft.setTextSize(2);
    tft.drawCentreString("FIRMWARE UPDATE", w / 2, 12, 1);

    tft.setTextSize(1);
    tft.setTextColor(C_DIM);
    tft.drawCentreString("DO NOT POWER OFF DEVICE", w / 2, 58, 1);

    tft.setTextColor(C_TEXT);
    tft.setTextSize(2);
    tft.drawCentreString("Flashing...", w / 2, 85, 1);

    // Progress bar frame
    int barX = 20;
    int barY = 125;
    int barW = w - 40;
    int barH = 22;
    tft.drawRect(barX, barY, barW, barH, C_BORDER);

    tft.setTextSize(1);
    tft.setTextColor(C_DIM);
    tft.drawString("0%", barX, barY + 28);
}

void drawOtaProgress(int percent) {
    int w = tft.width();
    int barX = 20;
    int barY = 125;
    int barW = w - 40;
    int barH = 22;

    int fillW = ((barW - 2) * percent) / 100;
    if (fillW > 0) {
        tft.fillRect(barX + 1, barY + 1, fillW, barH - 2, C_ACCENT);
    }
    if ((barW - 2) - fillW > 0) {
        tft.fillRect(barX + 1 + fillW, barY + 1, (barW - 2) - fillW, barH - 2, C_BG);
    }

    // Percentage text
    tft.fillRect(barX, barY + 28, 80, 15, C_BG);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    tft.setTextSize(1);
    tft.setTextColor(percent == 100 ? C_ACCENT : C_DIM);
    tft.drawString(buf, barX, barY + 28);
}

void drawOtaSuccess() {
    int w = tft.width();
    int h = tft.height();

    tft.fillScreen(C_BG);
    tft.fillRect(0, 0, w, 2, C_ACCENT);

    tft.setTextSize(1);
    tft.setTextColor(C_ACCENT);
    tft.drawCentreString("UPDATE COMPLETE", w / 2, 70, 1);

    tft.setTextSize(3);
    tft.setTextColor(C_TEXT);
    tft.drawCentreString("SUCCESS", w / 2, 95, 1);

    tft.setTextSize(1);
    tft.setTextColor(C_DIM);
    tft.drawCentreString("Rebooting system...", w / 2, 145, 1);

    tft.fillRect(0, h - 2, w, 2, C_ACCENT);
}

void drawOtaError(const char* errMsg) {
    int w = tft.width();
    int h = tft.height();

    tft.fillScreen(C_BG);
    tft.fillRect(0, 0, w, 2, C_ALERT);

    tft.setTextSize(1);
    tft.setTextColor(C_ALERT);
    tft.drawCentreString("UPDATE FAILED", w / 2, 70, 1);

    tft.setTextSize(2);
    tft.setTextColor(C_TEXT);
    tft.drawCentreString("ERROR", w / 2, 95, 1);

    tft.setTextSize(1);
    tft.setTextColor(C_DIM);
    tft.drawCentreString(errMsg ? errMsg : "Unknown error", w / 2, 130, 1);
    tft.drawCentreString("Rebooting in 3s...", w / 2, 150, 1);

    tft.fillRect(0, h - 2, w, 2, C_ALERT);
}
