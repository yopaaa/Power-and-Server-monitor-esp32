#include "LCD.h"
#include <Arduino.h>
#include <math.h>

#define SCREEN_W 240
#define SCREEN_H 240

int centerX = SCREEN_W / 2;
int centerY = SCREEN_H / 2;
int radius = 100;

int prevSecX, prevSecY;
int prevMinX, prevMinY;
int prevHourX, prevHourY;

unsigned long lastTick = 0;
unsigned long lastDialRefresh = 0;
int seconds = 0;
int minutes = 0;
int hours = 10;

void drawNumber()
{
    for (int i = 1; i <= 12; i++) {
        float angle = i * 30 * DEG_TO_RAD;
        int x = centerX + sin(angle) * (radius - 20);
        int y = centerY - cos(angle) * (radius - 20);

        tft.setTextColor(TFT_WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(String(i), x, y, 1);
    }
}

void drawDial()
{
    tft.fillScreen(TFT_BLACK);

    for (int i = 0; i < 2; i++) {
        tft.drawCircle(centerX, centerY, radius - i, TFT_WHITE);
    }

    drawNumber();
}

void drawHand(float angleDeg, int length, uint16_t color, int &prevX,
              int &prevY)
{
    float angle = angleDeg * DEG_TO_RAD;

    int x = centerX + sin(angle) * length;
    int y = centerY - cos(angle) * length;

    // hapus jarum lama
    tft.drawLine(centerX, centerY, prevX, prevY, TFT_BLACK);

    // gambar jarum baru
    tft.drawLine(centerX, centerY, x, y, color);

    prevX = x;
    prevY = y;
}

void updateClock()
{
    unsigned long currentMillis = millis();

    // 1. Cek Refresh Dial setiap 30 detik (Non-blocking)
    if (currentMillis - lastDialRefresh >= 3000) {
        lastDialRefresh = currentMillis;
        drawNumber(); 
    }

    if (currentMillis - lastTick < 1000)
        return;

    lastTick = currentMillis;

    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);

    seconds = timeinfo->tm_sec;
    minutes = timeinfo->tm_min;
    hours = timeinfo->tm_hour % 12;

    // float secAngle = seconds * 6;
    float secAngle = (seconds + millis() % 1000 / 1000.0) * 6;
    float minAngle = minutes * 6 + seconds * 0.1;
    float hourAngle = hours * 30 + minutes * 0.5;

    drawHand(secAngle, radius - 10, TFT_RED, prevSecX, prevSecY);
    drawHand(minAngle, radius - 20, TFT_GREEN, prevMinX, prevMinY);
    drawHand(hourAngle, radius - 40, TFT_WHITE, prevHourX, prevHourY);
}

