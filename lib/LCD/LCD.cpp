#include "LCD.h"
#include <LittleFS.h>
#include <TJpg_Decoder.h>

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
    TJpgDec.setJpgScale(1);

    pinMode(LCD_BL_PIN, OUTPUT);
    lcdBacklight(false);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_WHITE);

    lcdBacklight(true);
    showCenterImage("/linux.jpg");
    startGif(20);
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

void showCenterImage(const char *Filename)
{
    uint16_t w, h;

    if (TJpgDec.getFsJpgSize(&w, &h, Filename, LittleFS) != 0)
        return;

    int x = (tft.width() - w) / 2;
    int y = (tft.height() - h) / 2;

    JRESULT res = TJpgDec.drawFsJpg(x, y, Filename, LittleFS);

    return;
}

