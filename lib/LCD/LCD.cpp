#include "LCD.h"
#include <TJpg_Decoder.h>
#include <LittleFS.h>

TFT_eSPI tft = TFT_eSPI();

#define LCD_BL_PIN 5

void lcdBacklight(bool on) { digitalWrite(LCD_BL_PIN, on ? HIGH : LOW); }

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    // if (y >= tft.height()) return false;
    tft.pushImage(x, y, w, h, bitmap);
    return true;
}

void lcdInit()
{
    TJpgDec.setCallback(tft_output);
    TJpgDec.setSwapBytes(true);

    pinMode(LCD_BL_PIN, OUTPUT);
    lcdBacklight(false);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_WHITE);

    lcdBacklight(true);
    showCenterImage("/linux.jpg");
}

void lcdClear(uint16_t color) { tft.fillScreen(color); }

void lcdPrint(String text, int x, int y, uint16_t color, uint8_t size)
{
    tft.setTextColor(color);
    tft.setTextSize(size);
    tft.setCursor(x, y);
    tft.print(text);
}

void showCenterImage(const char *Filename)
{
    uint16_t w, h;

    if (TJpgDec.getFsJpgSize(&w, &h, Filename, LittleFS) != 0)
        return;

    int x = (tft.width() - w) / 2;
    int y = (tft.height() - h) / 2;

    TJpgDec.drawFsJpg(x, y, Filename, LittleFS);
}