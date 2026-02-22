#include "LCD.h"
#include <LittleFS.h>
#include <TJpg_Decoder.h>



bool gifActive = true;
unsigned long gifLastFrame = 0;
int gifFrame = 0;
int gifMaxFrame = 45;
int gifFPS = 10; // frame per second

void startGif(int fps = 10)
{
    gifFPS = fps;
    gifFrame = 0;
    gifActive = true;
}
void stopGif()
{
    gifActive = false;
}
void gifLoop()
{
    if (!gifActive) return;

    unsigned long frameInterval = 1000 / gifFPS;

    if (millis() - gifLastFrame < frameInterval) return;

    gifLastFrame = millis();

    char path[32];
    sprintf(path, "/penguin/%d.jpg", gifFrame);

    showCenterImage(path);

    gifFrame++;
    if (gifFrame > gifMaxFrame)
        gifFrame = 0;
}