#include "Devices.h"
#include "SavedWifi.h"

String generateRandomID(int length)
{
    const char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";

    String result = "";

    for (int i = 0; i < length; i++) {
        int index = random(0, sizeof(charset) - 1);
        result += charset[index];
    }

    return result;
}

void initDeviceID()
{
    prefs.begin("device_store", false);

    if (prefs.isKey("device_id")) {
        _id = prefs.getString("device_id", "");
        Serial.println("Device ID loaded: " + _id);
    } else {
        randomSeed(esp_random());

        _id = generateRandomID(10);

        prefs.putString("device_id", _id);

        Serial.println("New Device ID created: " + _id);
    }

    prefs.end();
}