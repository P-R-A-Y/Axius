#ifndef AXIUSLINK_H
#define AXIUSLINK_H

#include <Arduino.h>
#include <vector>
#include "mod_class.h"
#include "axiusPackets.h"

class AxiusLink : public Mod {
public:
  String getName() override;
  void firsttick() override;
  void setup() override;
  void tick();

  void supertick();
  void onPacket(uint8_t* frame);
private:
  uint8_t state = 0, scursor = 0, sstartpos = 0, pcursor = 0, pstartpos = 0, ticksAfterLastPacket = 0;
  uint8_t beforeId = 255, beforesentid = 255;
  uint32_t lastRequestTime = 0;
  const uint8_t pingNearDevicesEvery_seconds = 5;
  std::vector<uint8_t*> bufferedPackets;
  struct Device {
    uint8_t id;
    String stype;
    unsigned long lastTimeSeen;
  };
  std::vector<Device> nearbyDevices;
  BeaconRequestPacket finalBeaconPacket;
  std::vector<String> parameters {"call restart()", "call tomenu()", "test", "test2", "test3"};
  std::vector<String> curparameters;

  void processPacket(uint8_t id, uint8_t* packet);
  void sendPacket(AxiusPacket* p);
};

#endif