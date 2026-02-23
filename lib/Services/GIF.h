#ifndef GIF_H
#define GIF_H

#include <Arduino.h>

extern bool gifActive;

void loadGifConfig();
void startGif(String folderName, int fps, int maxFrame);
void stopGif();
void gifLoop();

#endif