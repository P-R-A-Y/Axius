#pragma once
#include "compile_parameters.h"
#include <Arduino.h>
#include <string>

class AxiusSSD;

class Mod {
public:
  Mod(AxiusSSD* instance, uint16_t ID) : axius(instance), ID(ID) {};
  Mod() {};
  virtual void tick() = 0;
  virtual void firsttick() = 0;
  virtual void setup() = 0;
  virtual String getName() = 0;
  virtual bool isRebootable() { return false; };
  uint16_t getID() { return ID; };
  AxiusSSD* axius = nullptr;
private:
  uint16_t ID;
};

class About : public Mod {
public:
  About(AxiusSSD* parent) : Mod(parent) {};
  void tick() override;
  void firsttick() override;
  void setup() override;
  String getName() override;
};