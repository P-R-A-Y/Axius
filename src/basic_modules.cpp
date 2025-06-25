#include "basic_modules.h"

GyroscopeModule::GyroscopeModule() {
}

void GyroscopeModule::update() {
  if (!isConnected()) return;
  IMU.update();
  IMU.getAccel(&accelData);
  IMU.getGyro(&gyroData);
  temperature = IMU.getTemp();
  rotX = accelData.accelX;
  rotY = accelData.accelY;
  rotZ = accelData.accelZ;
  velX = gyroData.gyroX;
  velY = gyroData.gyroY;
  velZ = gyroData.gyroZ;
  //Serial.println("vx: "+String(velX)+"  vy: "+String(velY)+"  vz: "+String(velZ)+"  rx: "+String(rotX)+"  ry: "+String(rotY)+"  rz: "+String(rotZ));
  float deltaX = abs(velX - prevX);
  float deltaY = abs(velY - prevY);
  float deltaZ = abs(velZ - prevZ);
  prevX = velX;
  prevY = velY;
  prevZ = velZ;
  float shakeMagnitude = sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
  shakeLevel = mapShakeLevel(shakeMagnitude);
  //Serial.println(shakeLevel);
}

void GyroscopeModule::connect() {
  error = IMU.init(calib, 0x68);
  setConnected(error == 0);
  if (!isConnected()) {
    temperature = 28.0f;
  } else {
    //mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    //mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    //mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
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

void VoltmeterModule::disconnect() {
  ina219.powerSave(true);
}