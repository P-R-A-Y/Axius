#include "basic_modules.h"

GyroscopeModule::GyroscopeModule() {
}

void GyroscopeModule::update() {
  if (!isConnected()) return;
  mpu.getEvent(&rotation, &velocity, &t); //gyro and acceleration flipped cause im using unofficial module
  rotX = rotation.gyro.x;
  rotY = rotation.gyro.y;
  rotZ = rotation.gyro.z;
  velX = velocity.acceleration.x;
  velY = velocity.acceleration.y;
  velZ = velocity.acceleration.z;
  temperature = t.temperature;
  float deltaX = abs(rotation.acceleration.x - prevX);
  float deltaY = abs(rotation.acceleration.y - prevY);
  float deltaZ = abs(rotation.acceleration.z - prevZ);
  prevX = rotation.acceleration.x;
  prevY = rotation.acceleration.y;
  prevZ = rotation.acceleration.z;
  float shakeMagnitude = sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
  shakeLevel = mapShakeLevel(shakeMagnitude);
  
}

void GyroscopeModule::connect() {
  setConnected(mpu.begin());
  if (!isConnected()) {
    temperature = 28.0f;
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