#include "PowerLogger.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

static PowerLoggerConfig loggerCfg = {
    "",                         // endpointUrl
    "esp32_secret_token_123",   // apiKey
    15,                         // intervalSec
    false,                      // isEnabled
    0,                          // lastPush
    0,                          // lastHttpStatus
    "Belum pernah mengirim"     // lastPushMessage
};

static const char* CONFIG_FILE = "/power_logger.json";

void initPowerLogger()
{
    loadPowerLoggerConfigFromFS();
}

const PowerLoggerConfig& getPowerLoggerConfig()
{
    return loggerCfg;
}

void setPowerLoggerConfig(const String &url, const String &key, uint32_t intervalSec, bool enabled)
{
    loggerCfg.endpointUrl = url;
    loggerCfg.endpointUrl.trim();
    while (loggerCfg.endpointUrl.endsWith("/")) {
        loggerCfg.endpointUrl.remove(loggerCfg.endpointUrl.length() - 1);
    }

    loggerCfg.apiKey = key;
    loggerCfg.apiKey.trim();

    if (intervalSec < 5) intervalSec = 5;
    if (intervalSec > 3600) intervalSec = 3600;
    loggerCfg.intervalSec = intervalSec;

    loggerCfg.isEnabled = enabled;

    savePowerLoggerConfigToFS();
}

bool savePowerLoggerConfigToFS()
{
    File f = LittleFS.open(CONFIG_FILE, "w");
    if (!f) return false;

    JsonDocument doc;
    doc["endpointUrl"] = loggerCfg.endpointUrl;
    doc["apiKey"] = loggerCfg.apiKey;
    doc["intervalSec"] = loggerCfg.intervalSec;
    doc["isEnabled"] = loggerCfg.isEnabled;

    serializeJson(doc, f);
    f.close();
    return true;
}

bool loadPowerLoggerConfigFromFS()
{
    if (!LittleFS.exists(CONFIG_FILE)) return false;

    File f = LittleFS.open(CONFIG_FILE, "r");
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) return false;

    if (doc["endpointUrl"].is<const char*>()) loggerCfg.endpointUrl = doc["endpointUrl"].as<String>();
    if (doc["apiKey"].is<const char*>()) loggerCfg.apiKey = doc["apiKey"].as<String>();
    if (doc["intervalSec"].is<uint32_t>()) loggerCfg.intervalSec = doc["intervalSec"].as<uint32_t>();
    if (doc["isEnabled"].is<bool>()) loggerCfg.isEnabled = doc["isEnabled"].as<bool>();

    if (loggerCfg.intervalSec < 5) loggerCfg.intervalSec = 15;

    return true;
}

bool pushPowerMetrics(const PZEMMetrics &metrics, String &outErr)
{
    if (WiFi.status() != WL_CONNECTED) {
        outErr = "WiFi tidak terhubung";
        loggerCfg.lastHttpStatus = -1;
        loggerCfg.lastPushMessage = outErr;
        return false;
    }

    if (loggerCfg.endpointUrl.isEmpty()) {
        outErr = "Endpoint URL kosong";
        loggerCfg.lastHttpStatus = -2;
        loggerCfg.lastPushMessage = outErr;
        return false;
    }

    // Format payload JSON
    char payload[192];
    snprintf(payload, sizeof(payload),
        "{\"voltage\":%.1f,\"current\":%.2f,\"power\":%.1f,\"energy\":%.3f,\"frequency\":%.1f,\"pf\":%.2f}",
        metrics.voltage, metrics.current, metrics.power, metrics.energy, metrics.frequency, metrics.pf
    );

    HTTPClient http;
    http.begin(loggerCfg.endpointUrl);
    http.setTimeout(2500); // 2.5 detik max timeout agar loop LCD tetap lancar
    http.addHeader("Content-Type", "application/json");

    if (!loggerCfg.apiKey.isEmpty()) {
        http.addHeader("X-API-Key", loggerCfg.apiKey);
    }

    int httpCode = http.POST((uint8_t*)payload, strlen(payload));
    loggerCfg.lastPush = millis();
    loggerCfg.lastHttpStatus = httpCode;

    bool success = false;
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
        success = true;
        loggerCfg.lastPushMessage = "Sukses (HTTP " + String(httpCode) + ")";
        outErr = "";
    } else if (httpCode == 401) {
        loggerCfg.lastPushMessage = "Ditolak: API Key salah (HTTP 401)";
        outErr = loggerCfg.lastPushMessage;
    } else if (httpCode > 0) {
        loggerCfg.lastPushMessage = "Gagal: HTTP " + String(httpCode);
        outErr = loggerCfg.lastPushMessage;
    } else {
        loggerCfg.lastPushMessage = "Koneksi Gagal: " + http.errorToString(httpCode);
        outErr = loggerCfg.lastPushMessage;
    }

    http.end();
    return success;
}

bool testPowerLogger(const String &url, const String &key, String &outMsg)
{
    if (WiFi.status() != WL_CONNECTED) {
        outMsg = "ESP32 belum terhubung ke WiFi!";
        return false;
    }

    if (url.isEmpty()) {
        outMsg = "URL Endpoint tidak boleh kosong!";
        return false;
    }

    const PZEMMetrics &m = getPZEMMetrics();
    char payload[192];
    snprintf(payload, sizeof(payload),
        "{\"voltage\":%.1f,\"current\":%.2f,\"power\":%.1f,\"energy\":%.3f,\"frequency\":%.1f,\"pf\":%.2f}",
        m.isConnected ? m.voltage : 220.0f,
        m.isConnected ? m.current : 0.0f,
        m.isConnected ? m.power : 0.0f,
        m.isConnected ? m.energy : 0.0f,
        m.isConnected ? m.frequency : 50.0f,
        m.isConnected ? m.pf : 1.0f
    );

    HTTPClient http;
    http.begin(url);
    http.setTimeout(3000);
    http.addHeader("Content-Type", "application/json");

    if (!key.isEmpty()) {
        http.addHeader("X-API-Key", key);
    }

    int httpCode = http.POST((uint8_t*)payload, strlen(payload));
    bool ok = false;

    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
        ok = true;
        outMsg = "Berhasil terhubung dan data diterima! (HTTP 200 OK)";
    } else if (httpCode == 401) {
        outMsg = "Gagal: Otentikasi ditolak (HTTP 401 Unauthorized). Periksa X-API-Key!";
    } else if (httpCode > 0) {
        outMsg = "Server merespons error: HTTP " + String(httpCode);
    } else {
        outMsg = "Gagal menghubungi server: " + http.errorToString(httpCode);
    }

    http.end();
    return ok;
}

void handlePowerLoggerLoop(const PZEMMetrics &metrics)
{
    if (!loggerCfg.isEnabled || loggerCfg.endpointUrl.isEmpty()) {
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    unsigned long now = millis();
    unsigned long intervalMs = loggerCfg.intervalSec * 1000UL;

    if (now - loggerCfg.lastPush >= intervalMs) {
        String err;
        pushPowerMetrics(metrics, err);
        if (!err.isEmpty()) {
            Serial.printf("[PowerLogger] Push warning: %s\n", err.c_str());
        } else {
            Serial.println("[PowerLogger] Metrik berhasil dikirim ke server");
        }
    }
}
