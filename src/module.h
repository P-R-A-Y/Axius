#pragma once
#include <string>
#include <Arduino.h>

//iic modules only
class Module {
public:
  Module();
  virtual void update() = 0;
  virtual void connect() = 0;
  virtual String getName() = 0;
  bool isConnected() { return connected; };
  void setConnected(bool c) { connected = c; };
  bool isUpdated() {
    if (updated) {
      updated = false;
      return true;
    } else return false;
  }
  bool updated = false;
private:
  bool connected = false;
};