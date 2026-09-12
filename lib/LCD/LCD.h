#ifndef LCD_MODULE_H
#define LCD_MODULE_H

#include <TFT_eSPI.h>

void lcdInit();
void lcdClear(uint16_t color = TFT_BLACK);
void lcdPrint(String text, int x = 10, int y = 40, uint16_t color = TFT_RED, uint8_t size = 2);
void lcdBacklight(bool on);
void lcdBacklight(int brightness);
void lcdPrintCenterX(String text, int line, uint16_t color = TFT_BLACK, uint8_t size = 2);

// OTA Update Screens
void drawOtaStart();
void drawOtaProgress(int percent);
void drawOtaSuccess();
void drawOtaError(const char* errMsg);

extern TFT_eSPI tft;

#endif