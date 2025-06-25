#ifndef Link_H
#define Link_H

#include <vector>
#include "mod_class.h"
#include "axiusPackets.h"

struct Device {
  uint8_t id;
  String stype;
  uint32_t lastTimeSeen;
  int ss;
};

class Link : public Mod {
public:
  Link(AxiusSSD* axiusInstance, uint16_t ID) : Mod(axiusInstance, ID) {};
  String getName() override;
  void firsttick() override;
  void setup() override;
  void tick();

  void supertick();
  void onPacket(uint8_t* frame, int rssi);
  bool sendPacket(AxiusPacket* p);
  void sendSomeElectronsIntoTheAir(String deviceName, uint8_t deviceID);
  Device* getDevice(uint8_t id) {
    for (uint8_t i = 0; i < nearbyDevices.size(); i++) {
      if (nearbyDevices[i].id == id) return &nearbyDevices[i];
    }
    return nullptr;
  }
  std::vector<Device> nearbyDevices;
private:
  uint8_t state = 0, scursor = 0, sstartpos = 0, pcursor = 0, pstartpos = 0, ticksAfterLastPacket = 0;
  uint8_t beforeId = 255, beforesentid = 255;
  uint32_t lastRequestTime = 0;
  const uint8_t pingNearDevicesEvery_seconds = 5;
  std::vector<uint8_t*> bufferedPackets;
  std::vector<int> rssiPack;
  BeaconBroadcastPacket finalBeaconPacket;
  std::vector<String> parameters {"call restart()", "call tomenu()", "test", "test2", "test3"};
  std::vector<String> curparameters;

  void processPacket(uint8_t id, uint8_t* packet, int rssi);
  
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