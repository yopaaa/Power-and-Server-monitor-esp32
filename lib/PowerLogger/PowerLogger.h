#ifndef POWER_LOGGER_H
#define POWER_LOGGER_H

#include <Arduino.h>
#include "PZEMManager.h"

struct PowerLoggerConfig {
    String endpointUrl;       // Contoh: "http://192.168.1.100:8080/api/metrics"
    String apiKey;            // Kunci otentikasi X-API-Key
    uint32_t intervalSec;     // Interval pengiriman detik (5 - 300 detik)
    bool isEnabled;           // Status aktif/nonaktif
    unsigned long lastPush;   // Timestamp millis push terakhir
    int lastHttpStatus;       // HTTP code respon terakhir (200, 401, dll)
    String lastPushMessage;   // Keterangan status push
};

void initPowerLogger();
const PowerLoggerConfig& getPowerLoggerConfig();
void setPowerLoggerConfig(const String &url, const String &key, uint32_t intervalSec, bool enabled);
bool savePowerLoggerConfigToFS();
bool loadPowerLoggerConfigFromFS();

// Mengirimkan metrik daya ke endpoint server (non-blocking timeout 2500ms)
bool pushPowerMetrics(const PZEMMetrics &metrics, String &outErr);

// Tes pengiriman data ke server tanpa perlu menunggu interval
bool testPowerLogger(const String &url, const String &key, String &outMsg);

// Dipanggil di loop utama untuk mengecek jadwal pengiriman
void handlePowerLoggerLoop(const PZEMMetrics &metrics);

#endif // POWER_LOGGER_H
