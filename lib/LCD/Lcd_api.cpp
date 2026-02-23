#include "Lcd_api.h"
#include "LCD.h"
#include "Services.h"
#include "Slideshow.h"
#include <LittleFS.h>
#include <TJpg_Decoder.h>

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
        String folder =
            server.hasArg("folder") ? server.arg("folder") : currentSlideFolder;

        unsigned long delay =
            server.hasArg("delay") ? server.arg("delay").toInt() : slideDelay;

        setSlideshow(folder, delay);

        if (slideCount == 0) {
            server.send(
                404, "application/json",
                "{\"status\":\"error\", \"message\":\"No JPG files found in " +
                    folder + "\"}");
            return;
        }

        // startSlideshow();

        // 6. Kirim respon sukses beserta info jumlah file
        String response = "{\"status\":\"slideshow started\", \"folder\":\"" +
                          folder + "\", \"count\":" + String(slideCount) + "}";
        server.send(200, "application/json", response);
    });

    // ===== SELECT SERVICE =====
    server.on("/service/select", HTTP_GET, [&server]() {
        if (!server.hasArg("id")) {
            server.send(
                400, "application/json",
                "{\"error\":\"id required (1:Clock, 2:GIF, 3:Slideshow)\"}");
            return;
        }

        int id = server.arg("id").toInt();

        selectService(id);

        String serviceName = "Unknown";
        if (id == 1)
            serviceName = "Analog Clock";
        else if (id == 2)
            serviceName = "GIF Animation";
        else if (id == 3)
            serviceName = "Slideshow";

        String response =
            "{\"status\":\"service changed\", \"id\":" + String(id) +
            ", \"name\":\"" + serviceName + "\"}";
        server.send(200, "application/json", response);
    });
}
