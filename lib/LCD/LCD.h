#ifndef LCD_MODULE_H
#define LCD_MODULE_H

#include <TFT_eSPI.h>
#include "PZEMManager.h"

// ─────────────────────────────────────────────────────────────────────────────
// Design System - High-Contrast Color Hunt Palette:
// Base: #1b211a, #628141, #8bae66, #ebd5ab
// Optimized for TFT LCD Contrast & Deep Black Levels
// ─────────────────────────────────────────────────────────────────────────────
#define C_BG         0x0000   // Pure Pitch Black (Maximizes LCD contrast & eliminates backlight wash)
#define C_CARD       0x1162   // #121c12 Deep Forest Card Surface
#define C_BORDER     0x4B65   // #4a6c34 Crisp Visible Olive Border
#define C_PRIMARY    0x6408   // #628141 Olive Green
#define C_ACCENT     0x8E8C   // #8bae66 Vibrant Sage Green
#define C_ACCENT_HI  0x9FEA   // #9de754 High-Luminance Lime
#define C_TEXT       0xEEF5   // #ebd5ab Warm Cream for Labels & Headers
#define C_VALUE      0xFFFF   // Pure White for Big Readout Numbers (Maximum Contrast)
#define C_DIM        0x7BEF   // Crisp Neutral Muted Gray
#define C_ALERT      0xF9A6   // #f83434 High-Contrast Red Alert
#define C_BLACK      0x0000
#define C_WHITE      0xFFFF

void lcdInit();
void lcdClear(uint16_t color = C_BG);
void lcdPrint(String text, int x = 10, int y = 40, uint16_t color = TFT_RED, uint8_t size = 2);
void lcdBacklight(bool on);
void lcdBacklight(int brightness);
void lcdPrintCenterX(String text, int line, uint16_t color = C_TEXT, uint8_t size = 2);
void lcdToggleInversion();
bool lcdIsInverted();

// Power Meter Dashboard
void drawPowerMeterFrame();
void updatePowerMeterDisplay(const PZEMMetrics &m, const String &statusInfo = "");

// OTA Update Screens
void drawOtaStart();
void drawOtaProgress(int percent);
void drawOtaSuccess();
void drawOtaError(const char* errMsg);

extern TFT_eSPI tft;

#endif