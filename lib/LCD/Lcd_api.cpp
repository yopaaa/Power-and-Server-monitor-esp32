#include "LCD.h"
#include "Lcd_api.h"
#include <LittleFS.h>
#include <TJpg_Decoder.h>

extern TFT_eSPI tft;

// ===== Slideshow State =====
static bool slideshowActive = false;
static unsigned long slideDelay = 3000;
static unsigned long lastSlide = 0;
static int slideIndex = 0;
static String slideFiles[20];
static int slideCount = 0;


bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    // if (y >= tft.height()) return false;
    tft.pushImage(x, y, w, h, bitmap);
    return true;
}

void loadSlideFiles()
{
    slideCount = 0;
    Dir dir = LittleFS.openDir("/");

    while (dir.next() && slideCount < 20) {
        String name = dir.fileName();
        if (name.endsWith(".jpg")) {
            slideFiles[slideCount++] = name;
        }
    }

    slideIndex = 0;
}

void setupLcdApi(ESP8266WebServer &server)
{
    TJpgDec.setCallback(tft_output);
    TJpgDec.setSwapBytes(true);

    // ===== SHOW IMAGE =====
    server.on("/lcd/show", HTTP_GET, [&server]() {
        slideshowActive = false;

        if (!server.hasArg("img")) {
            server.send(400, "application/json",
                        "{\"error\":\"img required\"}");
            return;
        }

        String filename = "/" + server.arg("img");

        if (!LittleFS.exists(filename)) {
            server.send(404, "application/json",
                        "{\"error\":\"file not found\"}");
            return;
        }

        lcdClear();
        // TJpgDec.drawFsJpg(0, 0, filename);
        TJpgDec.drawFsJpg(0, 0, filename.c_str(), LittleFS);

        server.send(200, "application/json", "{\"status\":\"displayed\"}");
    });

    // ===== CLEAR =====
    server.on("/lcd/clear", HTTP_GET, [&server]() {
        slideshowActive = false;
        lcdClear();

        server.send(200, "application/json", "{\"status\":\"cleared\"}");
    });

    // ===== TEXT =====
    server.on("/lcd/text", HTTP_GET, [&server]() {
        slideshowActive = false;

        if (!server.hasArg("msg")) {
            server.send(400, "application/json",
                        "{\"error\":\"msg required\"}");
            return;
        }

        String msg = server.arg("msg");

        int x = server.hasArg("x") ? server.arg("x").toInt() : 10;
        int y = server.hasArg("y") ? server.arg("y").toInt() : 40;
        int size = server.hasArg("size") ? server.arg("size").toInt() : 2;

        lcdClear();
        lcdPrint(msg, x, y, TFT_RED, size);

        server.send(200, "application/json", "{\"status\":\"text displayed\"}");
    });

    // ===== SLIDESHOW =====
    server.on("/lcd/slideshow", HTTP_GET, [&server]() {
        slideDelay =
            server.hasArg("delay") ? server.arg("delay").toInt() : 3000;

        loadSlideFiles();

        if (slideCount == 0) {
            server.send(404, "application/json",
                        "{\"error\":\"no jpg files\"}");
            return;
        }

        slideshowActive = true;
        lastSlide = 0;

        server.send(200, "application/json",
                    "{\"status\":\"slideshow started\"}");
    });
}

void lcdApiLoop()
{
    if (!slideshowActive)
        return;

    if (millis() - lastSlide < slideDelay)
        return;

    lastSlide = millis();

    if (slideCount == 0)
        return;

    lcdClear();
    // TJpgDec.drawFsJpg(0, 0, slideFiles[slideIndex]);
    TJpgDec.drawFsJpg(0, 0, slideFiles[slideIndex].c_str(), LittleFS);

    slideIndex++;
    if (slideIndex >= slideCount)
        slideIndex = 0;
}