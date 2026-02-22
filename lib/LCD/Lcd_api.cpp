#include "Lcd_api.h"
#include "LCD.h"
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

void loadSlideFiles()
{
    slideCount = 0;
    // DIUBAH: ESP32 menggunakan open() pada "/" untuk iterasi file
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) {
        Serial.println("Gagal buka direktori root");
        return;
    }

    File file = root.openNextFile();
    while (file && slideCount < 20) {
        String name = String(file.name());
        // Di ESP32 file.name() terkadang sudah termasuk "/" di depan
        if (name.endsWith(".jpg") || name.endsWith(".JPG")) {
            // Pastikan path diawali "/" untuk TJpgDec
            if (!name.startsWith("/"))
                name = "/" + name;
            slideFiles[slideCount++] = name;
        }
        file = root.openNextFile();
    }

    slideIndex = 0;
    Serial.printf("Slideshow: %d file ditemukan\n", slideCount);
}

void setupLcdApi(WebServer &server)
{
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

        // lcdClear();
        showCenterImage(filename.c_str());

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

        // lcdClear();
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
    if (!slideshowActive || slideCount == 0)
        return;

    if (millis() - lastSlide < slideDelay)
        return;

    lastSlide = millis();

    lcdClear();
    // Gunakan .c_str() agar kompatibel dengan parameter const char*
    TJpgDec.drawFsJpg(0, 0, slideFiles[slideIndex].c_str(), LittleFS);

    slideIndex++;
    if (slideIndex >= slideCount)
        slideIndex = 0;
}