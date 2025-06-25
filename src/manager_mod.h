#ifndef MEM_H
#define MEM_H

#include "mod_class.h"
#include <string>
#include <Arduino.h>
#include <Wire.h>
#include <memory>
#include "common_utilities.h"
#include "image_storage.h"

#define JP 0
#define BOOLP 1
#define BYTEP 2
#define FLOATP 3
#define PETP 4
#define RMEMP 5

class Parameter {
public:
  virtual ~Parameter() = default;
  virtual uint16_t getBytesLength() = 0;
  //virtual void setValue(int input) = 0;
  uint16_t address = 0;
  String name;
  virtual int classType() = 0;
  virtual String toString() = 0;
};

class BoolParameter : public Parameter {
public:
  bool defaultv;
  bool curval;
  BoolParameter(String name, bool defaultv) : defaultv(defaultv), curval(defaultv) {
    this->name = name;
  }
  uint16_t getBytesLength() override {
    return 1;
  }
  int classType() override { return BOOLP; }
  String toString() override {
    return name+": "+(curval ? "true" : "false");
  }
};

class ByteParameter : public Parameter {
public:
  byte defaultv;
  byte curval;
  ByteParameter(String name, byte defaultv) : defaultv(defaultv), curval(defaultv) {
    this->name = name;
  }
  uint16_t getBytesLength() override {
    return 1;
  }
  int classType() override { return BYTEP; }
  String toString() override {
    return name+": "+String(curval);
  }
};

class FloatParameter : public Parameter {
public:
  float defaultv;
  float curval;
  FloatParameter(String name, float defaultv) : defaultv(defaultv), curval(defaultv) {
    this->name = name;
  }
  uint16_t getBytesLength() override {
    return 4;
  }
  int classType() override { return FLOATP; }
  String toString() override {
    return name+": "+String(curval);
  }
};

class PetDedicatedMemory : public Parameter {
public:
  uint16_t size;
  bool broken = false;
  PetDedicatedMemory(){
    this->name = "undefined";
  }
  uint16_t getBytesLength() override {
    return 15360 + 2; //15kb + 2bytes(model size)
  }
  int classType() override { return PETP; }
  String toString() override {
    return name;
  }
};

class ReservedMemory : public Parameter {
public:
  uint16_t size;
  ReservedMemory(String name, uint16_t reSize){
    this->name = name+"["+String(reSize)+"]";
    this->size = reSize;
  }
  uint16_t getBytesLength() override {
    return size;
  }
  int classType() override { return RMEMP; }
  String toString() override {
    return name;
  }
};

class ManagerMod : public Mod {
public:
  ManagerMod(AxiusSSD* axiusInstance, uint16_t ID) : Mod(axiusInstance, ID) {};
  void setup() override;
  String getName() override { return "System"; };
  void tick() override;
  void firsttick() override;

  void writeEEPROM(uint16_t eeaddress, byte data);
  uint8_t readEEPROM(uint16_t eeaddress);
  void readSeveralBytesEEPROM(uint16_t address, uint16_t numbytes, uint8_t *targetBuffer, uint16_t targetBufferOffset);
  uint8_t* readSeveralBytesEEPROM(uint16_t eeaddress, uint16_t numbytes);
  void writeSeveralBytesEEPROM(uint16_t eeaddress, const uint8_t* data, uint16_t numbytes);
  void onMemoryError();
  bool getParameterBool         (const String &alias                   );
  void setParameterBool         (const String &alias, const byte value );
  uint8_t getParameterByte      (const String &alias,     uint8_t value);
  void setParameterByte         (const String &alias,     uint8_t value);
  void setParameterFloat        (const String &alias, const float value);
  float getParameterFloat       (const String &alias                   );
  uint16_t getAddresOfParameter (const String &alias                   );
  uint16_t getBytearraySize     (const String &alias                   );
  void updateIcon();

  const uint8_t EEPROM_ADDR = 0x50;
  uint8_t error = 0;

  BoolParameter  exit       {"exit", false};
  BoolParameter  version    {"v1?", true};
  ByteParameter  cs         {"cursor", 0};
  ByteParameter  devid      {"deviceId", uint8_t(random(256))};
  /*
  special ids:
  0 - 
  200 - device in beacon mode
  254 - 
  255 - eeprom error
  */
  BoolParameter  mbu        {"modBackUp", false};
  BoolParameter  cim        {"crashedInMod", false};
  BoolParameter  ucf        {"useCursedFont", false};
  ByteParameter  contrast   {"displayContrast", 100};
  BoolParameter  beaconmode {"isInBeaconMode", false};
  ReservedMemory res        {"reserved", 9};

  std::vector<Parameter*> settings { &exit, &version, &cs, &devid, &mbu, &cim, &ucf, &contrast, &beaconmode, &res };

  IconProvider icon;
  void checkmem();
private:
  
  void loadSavedData();

  uint32_t lastMemRead = 0, lastMemWrite = 0;

  uint16_t memcheckaddr = 0, memcheckaddrmax = 0, curaddress = 0;
  bool fixedOnParameter = false, dim = false, memoryWorking = false;
  uint8_t state = 0, cursor = 0, startpos = 0, mcursor = 0, mstartpos = 0;
  std::vector<String> statepick {"exit", "parameters", "restart", "eeprom test", "modules", "switch font", "contrast: ", "switch dimming", "keyboard test", "RAM rot: x", "", "beacon mode" };
  bool ignoreBrokenMemory = false;

  const uint8_t* IMAGEBUFFER_1[5] = {
    diskgood, disknotconnected, diskread, diskwrite, diskreadwrite
  };
};


#endif