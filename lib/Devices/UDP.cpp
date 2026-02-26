#include <WiFi.h>
#include <WiFiUdp.h>
#include "Devices.h"

WiFiUDP udp;
const unsigned int DISCOVERY_PORT = 4210;

char incomingPacket[255];


// ======================
// INIT UDP
// ======================
void initUDPDiscovery() {
  udp.begin(DISCOVERY_PORT);
  Serial.print("UDP Discovery aktif di port ");
  Serial.println(DISCOVERY_PORT);
}


// ======================
// HANDLE DISCOVERY
// Panggil di loop()
// ======================
void handleUDPDiscovery() {
  int packetSize = udp.parsePacket();
  if (!packetSize) return;

  int len = udp.read(incomingPacket, 255);
  if (len <= 0) return;

  incomingPacket[len] = 0;

  if (String(incomingPacket) == "DISCOVER_ESP32") {
    String response = "ESP32_FOUND;";
    response += "IP=" + WiFi.localIP().toString();
    response += ";MAC=" + WiFi.macAddress();
    response += ";NAME=ESP32_Device";
    response += ";VERSION=" + String(VERSION);
    response += ";ID=" + _id;

    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.print(response);
    udp.endPacket();

    Serial.println("Discovery response dikirim");
  }
}