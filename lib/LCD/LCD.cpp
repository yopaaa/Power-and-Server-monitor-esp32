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
    tft.fillScreen(TFT_BLACK);

    lcdBacklight(true);
    lcdPrintCenterX("Power Meter", 2, TFT_CYAN, 2);
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
    // 1. Tentukan tinggi baris (Font size 2 = 16px + 4px padding = 20px)
    int lineHeight = (size * 8) + 4; 
    
    // 2. Hitung koordinat Y berdasarkan nomor baris
    // Baris 0 akan ada di y=2, Baris 1 di y=22, dst.
    int y = (line * lineHeight) + 2;

    // 3. Hitung koordinat X agar teks di tengah
    tft.setTextSize(size);
    int textWidth = tft.textWidth(text);
    int x = (tft.width() - textWidth) / 2;
    
    if (x < 0) x = 0; // Cegah teks keluar layar kiri

    // 4. Gambar teks
    tft.setTextColor(color);
    tft.setCursor(x, y);
    tft.print(text);
}

// ─────────────────────────────────────────────────────────────────────────────
// OTA Update Screens (Matching Ph-monitoring NVIDIA-style design)
// ─────────────────────────────────────────────────────────────────────────────
#define C_BLACK      0x0000
#define C_WHITE      0xFFFF
#define C_PRIMARY    tft.color565(118, 185,   0)   // #76b900 NVIDIA Green
#define C_HAIRLINE   tft.color565( 46,  46,  46)   // #2e2e2e
#define C_MUTE       tft.color565(117, 117, 117)   // #757575
#define C_ERROR      tft.color565(229,  32,  32)   // #e52020

void drawOtaStart() {
    int w = tft.width();

    tft.fillScreen(C_BLACK);
    tft.fillRect(0, 0, w, 2, C_PRIMARY);

    // Header
    tft.fillRect(0, 2, w, 36, C_BLACK);
    tft.drawFastHLine(0, 38, w, C_HAIRLINE);
    tft.setTextColor(C_WHITE);
    tft.setTextSize(2);
    tft.drawCentreString("FIRMWARE UPDATE", w / 2, 12, 1);

    tft.setTextSize(1);
    tft.setTextColor(C_MUTE);
    tft.drawCentreString("DO NOT POWER OFF DEVICE", w / 2, 58, 1);

    tft.setTextColor(C_WHITE);
    tft.setTextSize(2);
    tft.drawCentreString("Flashing...", w / 2, 85, 1);

    // Progress bar frame
    int barX = 20;
    int barY = 125;
    int barW = w - 40;
    int barH = 22;
    tft.drawRect(barX, barY, barW, barH, C_HAIRLINE);

    tft.setTextSize(1);
    tft.setTextColor(C_MUTE);
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
        tft.fillRect(barX + 1, barY + 1, fillW, barH - 2, C_PRIMARY);
    }
    if ((barW - 2) - fillW > 0) {
        tft.fillRect(barX + 1 + fillW, barY + 1, (barW - 2) - fillW, barH - 2, C_BLACK);
    }

    // Percentage text
    tft.fillRect(barX, barY + 28, 80, 15, C_BLACK);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", percent);
    tft.setTextSize(1);
    tft.setTextColor(percent == 100 ? C_PRIMARY : C_MUTE);
    tft.drawString(buf, barX, barY + 28);
}

void drawOtaSuccess() {
    int w = tft.width();
    int h = tft.height();

    tft.fillScreen(C_BLACK);
    tft.fillRect(0, 0, w, 2, C_PRIMARY);

    tft.setTextSize(1);
    tft.setTextColor(C_PRIMARY);
    tft.drawCentreString("UPDATE COMPLETE", w / 2, 70, 1);

    tft.setTextSize(3);
    tft.setTextColor(C_WHITE);
    tft.drawCentreString("SUCCESS", w / 2, 95, 1);

    tft.setTextSize(1);
    tft.setTextColor(C_MUTE);
    tft.drawCentreString("Rebooting system...", w / 2, 145, 1);

    tft.fillRect(0, h - 2, w, 2, C_PRIMARY);
}

void drawOtaError(const char* errMsg) {
    int w = tft.width();
    int h = tft.height();

    tft.fillScreen(C_BLACK);
    tft.fillRect(0, 0, w, 2, C_ERROR);

    tft.setTextSize(1);
    tft.setTextColor(C_ERROR);
    tft.drawCentreString("UPDATE FAILED", w / 2, 70, 1);

    tft.setTextSize(2);
    tft.setTextColor(C_WHITE);
    tft.drawCentreString("ERROR", w / 2, 95, 1);

    tft.setTextSize(1);
    tft.setTextColor(C_MUTE);
    tft.drawCentreString(errMsg ? errMsg : "Unknown error", w / 2, 130, 1);
    tft.drawCentreString("Rebooting in 3s...", w / 2, 150, 1);

    tft.fillRect(0, h - 2, w, 2, C_ERROR);
}


