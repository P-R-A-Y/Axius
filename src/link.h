#ifndef Link_H
#define Link_H

#include <Arduino.h>
#include <vector>
#include "mod_class.h"
#include "axiusPackets.h"

class Link : public Mod {
public:
  String getName() override;
  void firsttick() override;
  void setup() override;
  void tick();

  void supertick();
  void onPacket(uint8_t* frame, int rssi);
private:
  uint8_t state = 0, scursor = 0, sstartpos = 0, pcursor = 0, pstartpos = 0, ticksAfterLastPacket = 0;
  uint8_t beforeId = 255, beforesentid = 255;
  uint32_t lastRequestTime = 0;
  const uint8_t pingNearDevicesEvery_seconds = 5;
  std::vector<uint8_t*> bufferedPackets;
  std::vector<int> rssiPack;
  struct Device {
    uint8_t id;
    String stype;
    uint32_t lastTimeSeen;
    int ss;
  };
  std::vector<Device> nearbyDevices;
  BeaconBroadcastPacket finalBeaconPacket;
  std::vector<String> parameters {"call restart()", "call tomenu()", "test", "test2", "test3"};
  std::vector<String> curparameters;

  void processPacket(uint8_t id, uint8_t* packet, int rssi);
  void sendPacket(AxiusPacket* p);
  
  void updateDevice(uint8_t id, String name, int rssi) {
    if (nearbyDevices.size() > 0) {
      for (auto dev = nearbyDevices.begin(); dev != nearbyDevices.end(); dev++) {
        if (dev->id == id) {
          dev->lastTimeSeen = millis();
          dev->ss = rssi;
          return;
        }
      }
    }
    Device d;
    d.id = id;
    d.lastTimeSeen = millis();
    d.stype = name;
    d.ss = rssi;
    nearbyDevices.push_back(d);
  }
};

#endif