#ifndef LCD_API_H
#define LCD_API_H

#include <WebServer.h>

void setupLcdApi(WebServer &server);
void lcdApiLoop();

#endif