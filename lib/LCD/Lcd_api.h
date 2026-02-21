#ifndef LCD_API_H
#define LCD_API_H

#include <ESP8266WebServer.h>

void setupLcdApi(ESP8266WebServer &server);
void lcdApiLoop();   // untuk slideshow non-blocking

#endif