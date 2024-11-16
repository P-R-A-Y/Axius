#include "module.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_INA219.h>

class GyroscopeModule : public Module {
public:
  GyroscopeModule();
  void update() override;
  void connect() override;
  float getShake() { return shakeLevel; };
  String getName() override {
    if (isConnected())  return "[+] Gyro "+String(temperature)+"C S"+String(shakeLevel);
    else                return "[-] Gyroscope";
  };
  float temperature;
  float rotX, rotY, rotZ, velX, velY, velZ;
private:
  sensors_event_t t, rotation, velocity;
  Adafruit_MPU6050 mpu;
  float prevX = 0, prevY = 0, prevZ = 0, shakeLevel = 0;
  const float maxShakeThreshold = 50.0;
  const float minShakeThreshold = 0.05;
  float mapShakeLevel(float magnitude) {
    if (magnitude < minShakeThreshold) {
      return 0;
    } else if (magnitude > maxShakeThreshold) {
      return 1;
    }

    return (magnitude - minShakeThreshold) / (maxShakeThreshold - minShakeThreshold);
  }
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