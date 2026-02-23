#ifndef SLIDESHOW_H
#define SLIDESHOW_H

#include <Arduino.h>

extern bool slideshowActive;
extern String currentSlideFolder;
extern int slideCount;
extern unsigned long slideDelay;

void setSlideshow(String folder, unsigned long delayMs);
void loadSlideshowConfig();            
void loadSlideFiles(String folderName); 
void slideshowLoop();
void startSlideshow();

#endif