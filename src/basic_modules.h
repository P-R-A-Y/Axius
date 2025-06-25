#include "module.h"
#include "FastIMU.h"
#include <Adafruit_INA219.h>

class GyroscopeModule : public Module {
public:
  GyroscopeModule();
  void update() override;
  void connect() override;
  void disconnect() override {};
  float getShake() { return shakeLevel; };
  String getName() override {
    if (isConnected())  return "[+] Gyroscope "+String(temperature)+"C S"+String(shakeLevel);
    else                return "[-] Gyroscope [error: "+String(error)+"]";
  };
  float temperature;
  float rotX, rotY, rotZ, velX, velY, velZ;
private:
  MPU6050 IMU;
  calData calib = { 0 };
  AccelData accelData;
  GyroData gyroData;
  MagData magData;
  float prevX = 0, prevY = 0, prevZ = 0, shakeLevel = 0;
  const float maxShakeThreshold = 500.0;
  const float minShakeThreshold = 0.05;
  int error;
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
  void disconnect() override;
  String getName() override {
    if (isConnected())  return "[+] Voltmeter "+String(voltage)+"V";
    else                return "[-] Voltmeter";
  };
  
  float getVoltage() { return voltage; };
private:
  float voltage = -1.0;
  Adafruit_INA219 ina219;
};