#ifndef LCD_MODULE_H
#define LCD_MODULE_H

#include <TFT_eSPI.h>
#include "PZEMManager.h"

// ─────────────────────────────────────────────────────────────────────────────
// Design System - Color Hunt Palette:
// #1b211a, #628141, #8bae66, #ebd5ab
// ─────────────────────────────────────────────────────────────────────────────
#define C_BG         0x18C3   // #1b211a Dark Canvas
#define C_CARD       0x2144   // #222a21 Card Surface
#define C_BORDER     0x3206   // #354233 Hairline Border
#define C_PRIMARY    0x6408   // #628141 Olive Green
#define C_ACCENT     0x8D6C   // #8bae66 Sage Green / Highlight
#define C_TEXT       0xEEF5   // #ebd5ab Warm Cream
#define C_DIM        0xBD51   // #b8a88a Muted Beige
#define C_ALERT      0xD225   // Red Alert
#define C_BLACK      0x0000
#define C_WHITE      0xFFFF

void lcdInit();
void lcdClear(uint16_t color = C_BG);
void lcdPrint(String text, int x = 10, int y = 40, uint16_t color = TFT_RED, uint8_t size = 2);
void lcdBacklight(bool on);
void lcdBacklight(int brightness);
void lcdPrintCenterX(String text, int line, uint16_t color = C_TEXT, uint8_t size = 2);

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