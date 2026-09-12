#ifndef PZEM_MANAGER_H
#define PZEM_MANAGER_H

#include <Arduino.h>

#ifndef PZEM_RX_PIN
#define PZEM_RX_PIN 16
#endif

#ifndef PZEM_TX_PIN
#define PZEM_TX_PIN 17
#endif

struct PZEMMetrics {
    float voltage;     // V
    float current;     // A
    float power;       // W
    float energy;      // kWh
    float frequency;   // Hz
    float pf;          // Power Factor
    bool isConnected;  // Valid reading received
};

void initPZEM();
bool readPZEM();
const PZEMMetrics& getPZEMMetrics();
void resetPZEMEnergy();

#endif // PZEM_MANAGER_H
