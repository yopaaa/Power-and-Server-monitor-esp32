#include "PZEMManager.h"
#include <PZEM004Tv30.h>

static PZEM004Tv30 *pzem = nullptr;
static PZEMMetrics currentMetrics = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, false};

void initPZEM() {
    Serial.printf("[PZEM] Initializing PZEM-004T on Serial2 (RX:%d, TX:%d)...\n", PZEM_RX_PIN, PZEM_TX_PIN);
    pzem = new PZEM004Tv30(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);
}

bool readPZEM() {
    if (!pzem) return false;

    float v = pzem->voltage();
    float i = pzem->current();
    float p = pzem->power();
    float e = pzem->energy();
    float f = pzem->frequency();
    float pf = pzem->pf();

    if (isnan(v) || isnan(i) || isnan(p)) {
        currentMetrics.isConnected = false;
        return false;
    }

    currentMetrics.voltage = v;
    currentMetrics.current = i;
    currentMetrics.power = p;
    currentMetrics.energy = isnan(e) ? 0.0f : e;
    currentMetrics.frequency = isnan(f) ? 50.0f : f;
    currentMetrics.pf = isnan(pf) ? 1.0f : pf;
    currentMetrics.isConnected = true;

    return true;
}

const PZEMMetrics& getPZEMMetrics() {
    return currentMetrics;
}

void resetPZEMEnergy() {
    if (pzem) {
        pzem->resetEnergy();
        Serial.println("[PZEM] Energy reset requested.");
    }
}
