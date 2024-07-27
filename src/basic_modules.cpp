#include "basic_modules.h"

GyroscopeModule::GyroscopeModule() {
}

void GyroscopeModule::update() {
  if (!isConnected()) return;
  mpu.getEvent(&rotation, &velocity, &t);
  temperature = t.temperature;
}

void GyroscopeModule::connect() {
  setConnected(mpu.begin());
  if (!isConnected()) {
    temperature = -999.99;
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
}



VoltmeterModule::VoltmeterModule() {
}

void VoltmeterModule::update() {
  if (!isConnected()) return;
  voltage = ina219.getBusVoltage_V();
}

void VoltmeterModule::connect() {
  setConnected(ina219.begin());
}