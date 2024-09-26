#pragma once
#include <string>
#include <Arduino.h>

class Mod {
public:
  Mod();
  virtual void tick() = 0;
  virtual void firsttick() = 0;
  virtual void setup() = 0;
  virtual String getName() = 0;
  virtual bool isRebootable() {return false;};
};

class About : public Mod {
public:
  About();
  void tick() override;
  void firsttick() override;
  void setup() override;
  String getName() override;
};