#ifndef MEM_H
#define MEM_H

#include "mod_class.h"
#include <string>
#include <Arduino.h>
#include <Wire.h>
#include <memory>

#define JP 0
#define BOOLP 1
#define BYTEP 2
#define FLOATP 3
#define PETP 4
#define RMEMP 5

class Parameter {
public:
  virtual ~Parameter() = default; // Деструктор
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

class MemoryManagerMod : public Mod {
public:
  MemoryManagerMod();
  void setup() override;
  String getName() override { return "Options"; };
  void tick() override;
  void firsttick() override;

  void writeEEPROM(uint16_t eeaddress, byte data);
  uint8_t readEEPROM(uint16_t eeaddress);
  void readSeveralBytesEEPROM(uint16_t address, uint16_t numbytes, uint8_t *targetBuffer, uint16_t targetBufferOffset);
  uint8_t* readSeveralBytesEEPROM(uint16_t eeaddress, uint16_t numbytes);
  void writeSeveralBytesEEPROM(uint16_t eeaddress, const uint8_t* data, uint16_t numbytes);
  void onMemoryError();
  bool getParameterBool       (const String &alias                   );
  void setParameterBool       (const String &alias, const byte value );
  uint8_t getParameterByte    (const String &alias,     uint8_t value);
  void setParameterByte       (const String &alias,     uint8_t value);
  void setParameterFloat      (const String &alias, const float value);
  float getParameterFloat     (const String &alias                   );
  uint16_t getBytearrayAddres (const String &alias                   );
  uint16_t getBytearraySize   (const String &alias                   );
  uint8_t getStateImage();

  const uint8_t EEPROM_ADDR = 0x50;
  byte error = 0;

  BoolParameter exit       {"exit", false};
  BoolParameter version    {"v1?", false};
  ByteParameter cs         {"cursor", 0};
  ByteParameter devid      {"deviceId", random(256)};

  std::vector<Parameter*> settings { &exit, &version, &cs, &devid };

private:
  void checkmem();
  void loadSavedData();

  bool memoryWorking = false, fixedOnParameter = false;
  uint8_t state = 0, cursor = 0, startpos = 0, iconanimstate = 1, mcursor = 0, mstartpos = 0;
  unsigned long diskIconAnimTime = 0;
  const uint8_t numbytes = 1; //help compiler with choice between uint8_t requestFrom(int, int); and uint8_t requestFrom(uint8_t, uint8_t);
  const std::vector<String> statepick {"exit", "parameters", "restart", "deepsleep test", "modules"};
};


#endif