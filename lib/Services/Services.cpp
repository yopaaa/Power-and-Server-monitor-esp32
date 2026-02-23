#include "Services.h"
#include "Analog_clock.h"
#include "GIF.h"
#include "LCD.h"
#include "SavedWifi.h"
#include "Slideshow.h"

int SelectedService = 0;

void loadService()
{
    prefs.begin("srv_config", true);               // Mode Read-only
    SelectedService = prefs.getInt("last_svc", 0); // Default 0 jika belum ada
    prefs.end();

    Serial.print("Loaded Service ID: ");
    Serial.println(SelectedService);

    if (SelectedService == 1) {
        drawDial();
    } else if (SelectedService == 2) {
        loadGifConfig();
        gifActive = true;
    } else if (SelectedService == 3) {
        startSlideshow();
    }
}

void selectService(int serviceId)
{
    SelectedService = serviceId;

    // Simpan ke Preferences
    prefs.begin("srv_config", false); // Mode Read-Write
    prefs.putInt("last_svc", serviceId);
    prefs.end();

    Serial.print("Service saved: ");
    Serial.println(serviceId);

    if (SelectedService == 1) {
        drawDial();
    } else if (SelectedService == 2) {
        loadGifConfig();
        gifActive = true;
    } else if (SelectedService == 3) {
        startSlideshow();
    }
}

void servicesLoop()
{
    if (SelectedService == 1) {
        updateClock();

        return;
    } else if (SelectedService == 2) {
        gifLoop();
        return;
    } else if (SelectedService == 3) {
        slideshowLoop();
    }
}