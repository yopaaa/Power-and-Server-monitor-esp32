#include "LCD.h"

TFT_eSPI tft = TFT_eSPI();
const int ledChannel = 0;
const int freq = 5000;
const int resolution = 8; // 8-bit (0-255)

void lcdBacklight(bool on) { digitalWrite(TFT_BL, on ? HIGH : LOW); }

void lcdBacklight(int brightness) {
    if (brightness < 0) brightness = 0;
    if (brightness > 255) brightness = 255;
    ledcWrite(ledChannel, brightness);
}

void lcdInit()
{
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(TFT_BL, ledChannel);

    lcdBacklight(false);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(C_BG);

    lcdBacklight(true);
    drawPowerMeterFrame();
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
    tft.fillScreen(C_BG);

    // ── Header (Y: 0..24) ──
    tft.fillRect(0, 0, 240, 2, C_ACCENT); // Top accent line
    tft.setTextSize(2);
    tft.setTextColor(C_TEXT, C_BG);
    tft.drawString("POWER METER", 12, 6);

    tft.drawFastHLine(0, 24, 240, C_BORDER);

    // ── Hero Card: Active Power (W) (Y: 28..92, H: 64) ──
    tft.fillRect(8, 28, 224, 64, C_CARD);
    tft.drawRect(8, 28, 224, 64, C_BORDER);
    tft.fillRect(8, 28, 4, 4, C_ACCENT);
    tft.setTextSize(1);
    tft.setTextColor(C_DIM, C_CARD);
    tft.drawString("ACTIVE POWER", 16, 33);
    tft.setTextSize(2);
    tft.setTextColor(C_ACCENT, C_CARD);
    tft.drawString("W", 195, 58);

    // ── Middle Row: Voltage & Current (Y: 96..152, H: 56) ──
    // Left: Voltage (V)
    tft.fillRect(8, 96, 108, 56, C_CARD);
    tft.drawRect(8, 96, 108, 56, C_BORDER);
    tft.fillRect(8, 96, 4, 4, C_ACCENT);
    tft.setTextSize(1);
    tft.setTextColor(C_DIM, C_CARD);
    tft.drawString("VOLTAGE", 16, 101);
    tft.setTextSize(2);
    tft.setTextColor(C_ACCENT, C_CARD);
    tft.drawString("V", 98, 124);

    // Right: Current (A)
    tft.fillRect(124, 96, 108, 56, C_CARD);
    tft.drawRect(124, 96, 108, 56, C_BORDER);
    tft.fillRect(124, 96, 4, 4, C_ACCENT);
    tft.setTextSize(1);
    tft.setTextColor(C_DIM, C_CARD);
    tft.drawString("CURRENT", 132, 101);
    tft.setTextSize(2);
    tft.setTextColor(C_ACCENT, C_CARD);
    tft.drawString("A", 214, 124);

    // ── Bottom Row: Energy & Freq/PF (Y: 156..212, H: 56) ──
    // Left: Energy (kWh)
    tft.fillRect(8, 156, 108, 56, C_CARD);
    tft.drawRect(8, 156, 108, 56, C_BORDER);
    tft.fillRect(8, 156, 4, 4, C_ACCENT);
    tft.setTextSize(1);
    tft.setTextColor(C_DIM, C_CARD);
    tft.drawString("ENERGY", 16, 161);
    tft.setTextSize(1);
    tft.setTextColor(C_ACCENT, C_CARD);
    tft.drawString("kWh", 88, 192);

    // Right: Frequency & Power Factor
    tft.fillRect(124, 156, 108, 56, C_CARD);
    tft.drawRect(124, 156, 108, 56, C_BORDER);
    tft.fillRect(124, 156, 4, 4, C_ACCENT);
    tft.setTextSize(1);
    tft.setTextColor(C_DIM, C_CARD);
    tft.drawString("GRID STATUS", 132, 161);

    // ── Footer Bar (Y: 216..239) ──
    tft.drawFastHLine(0, 216, 240, C_BORDER);
}

void updatePowerMeterDisplay(const PZEMMetrics &m, const String &statusInfo)
{
    char buf[20];

    // 1. Power (W)
    tft.fillRect(16, 48, 172, 36, C_CARD);
    if (!m.isConnected) {
        tft.setTextColor(C_DIM, C_CARD);
        tft.drawString("--.-", 20, 52, 4);
    } else {
        tft.setTextColor(C_TEXT, C_CARD);
        if (m.power >= 1000.0f) {
            snprintf(buf, sizeof(buf), "%.2f", m.power / 1000.0f);
            tft.drawString(buf, 20, 50, 4);
            tft.setTextSize(2);
            tft.setTextColor(C_ACCENT, C_CARD);
            tft.drawString("kW", 188, 58);
        } else {
            snprintf(buf, sizeof(buf), "%.1f", m.power);
            tft.drawString(buf, 20, 50, 4);
            tft.setTextSize(2);
            tft.setTextColor(C_ACCENT, C_CARD);
            tft.drawString("W ", 195, 58);
        }
    }

    // 2. Voltage (V)
    tft.fillRect(14, 115, 80, 32, C_CARD);
    if (!m.isConnected) {
        tft.setTextColor(C_DIM, C_CARD);
        tft.drawString("--.-", 16, 118, 4);
    } else {
        snprintf(buf, sizeof(buf), "%.1f", m.voltage);
        tft.setTextColor(C_TEXT, C_CARD);
        tft.drawString(buf, 16, 118, 4);
    }

    // 3. Current (A)
    tft.fillRect(130, 115, 80, 32, C_CARD);
    if (!m.isConnected) {
        tft.setTextColor(C_DIM, C_CARD);
        tft.drawString("--.--", 132, 118, 4);
    } else {
        snprintf(buf, sizeof(buf), "%.2f", m.current);
        tft.setTextColor(C_TEXT, C_CARD);
        tft.drawString(buf, 132, 118, 4);
    }

    // 4. Energy (kWh)
    tft.fillRect(14, 175, 72, 32, C_CARD);
    if (!m.isConnected) {
        tft.setTextColor(C_DIM, C_CARD);
        tft.drawString("--.-", 16, 178, 4);
    } else {
        if (m.energy < 100.0f) {
            snprintf(buf, sizeof(buf), "%.2f", m.energy);
        } else {
            snprintf(buf, sizeof(buf), "%.1f", m.energy);
        }
        tft.setTextColor(C_TEXT, C_CARD);
        tft.drawString(buf, 16, 178, 4);
    }

    // 5. Grid Status: Frequency & PF
    tft.fillRect(130, 175, 96, 34, C_CARD);
    if (!m.isConnected) {
        tft.setTextColor(C_DIM, C_CARD);
        tft.drawString("F: -- Hz", 132, 176, 2);
        tft.drawString("PF: ---", 132, 192, 2);
    } else {
        snprintf(buf, sizeof(buf), "F: %.1fHz", m.frequency);
        tft.setTextColor(C_TEXT, C_CARD);
        tft.drawString(buf, 132, 176, 2);

        snprintf(buf, sizeof(buf), "PF: %.2f", m.pf);
        tft.setTextColor(C_ACCENT, C_CARD);
        tft.drawString(buf, 132, 192, 2);
    }

    // 6. Footer Status Bar
    tft.fillRect(0, 218, 240, 22, C_BG);
    if (m.isConnected) {
        tft.setTextColor(C_ACCENT, C_BG);
        tft.drawString("* PZEM OK", 10, 222, 2);
    } else {
        tft.setTextColor(C_ALERT, C_BG);
        tft.drawString("! PZEM WAITING", 10, 222, 2);
    }

    if (statusInfo.length() > 0) {
        tft.setTextColor(C_DIM, C_BG);
        tft.drawRightString(statusInfo, 230, 224, 1);
    }
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
