#include "Lcd_api.h"
#include "LCD.h"

void setupLcdApi(WebServer &server)
{
    // ===== CLEAR =====
    server.on("/lcd/clear", HTTP_GET, [&server]() {
        lcdClear();
        server.send(200, "application/json", "{\"status\":\"cleared\"}");
    });

    // ===== BACKLIGHT (Range 0-255) =====
    server.on("/lcd/backlight", HTTP_GET, [&server]() {
        if (!server.hasArg("value")) {
            server.send(400, "application/json",
                        "{\"error\":\"value required (0-255)\"}");
            return;
        }

        int brightnessValue = server.arg("value").toInt();
        if (brightnessValue < 0) brightnessValue = 0;
        if (brightnessValue > 255) brightnessValue = 255;

        lcdBacklight(brightnessValue);

        String response = "{\"status\":\"brightness set\", \"value\":" + String(brightnessValue) + "}";
        server.send(200, "application/json", response);
    });
}

