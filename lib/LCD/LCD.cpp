#include "LCD.h"

TFT_eSPI tft = TFT_eSPI();

#define LCD_BL_PIN 16

void lcdBacklight(bool on)
{
    digitalWrite(LCD_BL_PIN, on ? HIGH : LOW);
}

void lcdInit()
{
    pinMode(LCD_BL_PIN, OUTPUT);
    lcdBacklight(false);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    lcdBacklight(true);
}

void lcdClear(uint16_t color)
{
    tft.fillScreen(color);
}

void lcdPrint(String text, int x, int y, uint16_t color, uint8_t size)
{
    tft.setTextColor(color, TFT_BLACK);
    tft.setTextSize(size);
    tft.setCursor(x, y);
    tft.print(text);
}