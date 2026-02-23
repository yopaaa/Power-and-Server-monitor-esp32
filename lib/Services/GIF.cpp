#include "LCD.h"
#include <LittleFS.h>
#include <TJpg_Decoder.h>
#include "GIF.h"
#include "SavedWifi.h" 

bool gifActive = false;
unsigned long gifLastFrame = 0;
int gifFrame = 0;
int gifMaxFrame = 45;
int gifFPS = 10;
String currentGifFolder = "penguin";

void loadGifConfig() {
    prefs.begin("gif_cfg", true); 
    currentGifFolder = prefs.getString("folder", "penguin");
    gifFPS = prefs.getInt("fps", 10);
    gifMaxFrame = prefs.getInt("max_f", 45);
    prefs.end();
    
    Serial.printf("GIF Loaded: Folder=%s, FPS=%d\n", currentGifFolder.c_str(), gifFPS);
}

void startGif(String folderName, int fps, int maxFrame)
{
    currentGifFolder = folderName;
    gifFPS = fps;
    gifMaxFrame = maxFrame;
    gifFrame = 0;
    gifActive = true;

    // --- SIMPAN KE PREFERENCES ---
    prefs.begin("gif_cfg", false); // Mode Read-Write
    prefs.putString("folder", folderName);
    prefs.putInt("fps", fps);
    prefs.putInt("max_f", maxFrame);
    prefs.end();
    
    Serial.println("GIF Config saved to Preferences");
}

void stopGif() {
    gifActive = false;
}

void gifLoop() {
    if (!gifActive) return;

    unsigned long frameInterval = 1000 / gifFPS;
    if (millis() - gifLastFrame < frameInterval) return;
    gifLastFrame = millis();

    char path[64];
    sprintf(path, "/%s/%d.jpg", currentGifFolder.c_str(), gifFrame);

    showCenterImage(path);

    gifFrame++;
    if (gifFrame > gifMaxFrame)
        gifFrame = 0;
}