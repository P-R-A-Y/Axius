#include "module.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_INA219.h>

class GyroscopeModule : public Module {
public:
  GyroscopeModule();
  void update() override;
  void connect() override;
  String getName() override {
    if (isConnected())  return "[+] Gyroscope "+String(temperature)+"C";
    else                return "[-] Gyroscope";
  };
  sensors_event_t rotation, velocity;
  float temperature;
private:
  sensors_event_t t;
  Adafruit_MPU6050 mpu;
};

class VoltmeterModule : public Module {
public:
  VoltmeterModule();
  void update() override;
  void connect() override;
  String getName() override {
    if (isConnected())  return "[+] Voltmeter "+String(voltage)+"V";
    else                return "[-] Voltmeter";
  };
  
  float getVoltage() { return voltage; };
private:
  float voltage = -1.0;
  Adafruit_INA219 ina219;
};