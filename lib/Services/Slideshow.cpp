#include "Slideshow.h"
#include "Lcd_api.h"
#include "LCD.h"
#include "SavedWifi.h" // Menggunakan objek 'prefs' global
#include <LittleFS.h>
#include <TJpg_Decoder.h>

// ===== Slideshow State =====
static bool slideshowActive = false;
 unsigned long slideDelay = 3000;
static unsigned long lastSlide = 0;
static int slideIndex = 0;
static String slideFiles[20];
 int slideCount = 0;
String currentSlideFolder = "/slides"; // Folder default

void setSlideshow(String folder, unsigned long delayMs) {
    currentSlideFolder = folder;
    if (!currentSlideFolder.startsWith("/")) {
        currentSlideFolder = "/" + currentSlideFolder;
    }
    slideDelay = delayMs;

    prefs.begin("slide_cfg", false);
    prefs.putString("folder", currentSlideFolder);
    prefs.putUInt("delay", slideDelay);
    prefs.end();

    loadSlideFiles(currentSlideFolder);

    slideIndex = 0;
    lastSlide = millis(); // Reset timer agar gambar pertama langsung muncul atau mulai hitung dari nol
    
    Serial.printf("Slideshow updated: Folder %s, Delay %lu ms\n", currentSlideFolder.c_str(), slideDelay);
}

void loadSlideshowConfig() {
    prefs.begin("slide_cfg", true);
    currentSlideFolder = prefs.getString("folder", "/slides");
    slideDelay = prefs.getUInt("delay", 3000);
    prefs.end();
    
    Serial.printf("Slideshow Config: Folder=%s, Delay=%lu ms\n", currentSlideFolder.c_str(), slideDelay);
}

// --- FUNGSI: SCAN FOLDER TERTENTU ---
void loadSlideFiles(String folderName) {
    currentSlideFolder = folderName;
    
    // Pastikan folder diawali "/"
    if (!currentSlideFolder.startsWith("/")) {
        currentSlideFolder = "/" + currentSlideFolder;
    }

    slideCount = 0;
    File root = LittleFS.open(currentSlideFolder);
    
    if (!root || !root.isDirectory()) {
        Serial.printf("Gagal buka direktori: %s\n", currentSlideFolder.c_str());
        return;
    }

    File file = root.openNextFile();
    while (file && slideCount < 20) {
        String name = String(file.name()); // Di ESP32 ini biasanya path lengkap
        
        if (name.endsWith(".jpg") || name.endsWith(".JPG")) {
            // Jika name hanya nama file, gabungkan dengan folder
            if (!name.startsWith("/")) {
                slideFiles[slideCount++] = currentSlideFolder + "/" + name;
            } else {
                slideFiles[slideCount++] = name;
            }
        }
        file = root.openNextFile();
    }

    slideIndex = 0;
    
    // Simpan folder & delay ke Preferences tiap kali ganti folder
    prefs.begin("slide_cfg", false);
    prefs.putString("folder", currentSlideFolder);
    prefs.putUInt("delay", slideDelay);
    prefs.end();

    Serial.printf("Slideshow: %d file ditemukan di %s\n", slideCount, currentSlideFolder.c_str());
}

void startSlideshow() {
    loadSlideshowConfig(); 
    loadSlideFiles(currentSlideFolder);
    slideshowActive = true;
    lastSlide = millis(); 
}

void slideshowLoop() {
    if (!slideshowActive || slideCount == 0)
        return;

    if (millis() - lastSlide < slideDelay)
        return;

    lastSlide = millis();

    // Gunakan fungsi gambar yang sudah ada
    showCenterImage(slideFiles[slideIndex].c_str());

    slideIndex++;
    if (slideIndex >= slideCount)
        slideIndex = 0;
}