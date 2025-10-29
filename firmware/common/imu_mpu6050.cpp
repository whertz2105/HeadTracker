#include "imu_mpu6050.h"

// Register addresses used by the MPU-6050.
namespace {
constexpr uint8_t kRegPwrMgmt1 = 0x6B;
constexpr uint8_t kRegAccelConfig = 0x1C;
constexpr uint8_t kRegGyroConfig = 0x1B;
constexpr uint8_t kRegAccelStart = 0x3B;  // Start of accel data block
constexpr uint8_t kRegGyroStart = 0x43;   // Start of gyro data block
}

ImuMpu6050::ImuMpu6050(const char* label) : label_(label) {}

bool ImuMpu6050::begin(TwoWire& bus, uint8_t address) {
  bus_ = &bus;
  address_ = address;
  bus_->begin(Pins::kI2CSda, Pins::kI2CScl, 400000);  // Fast mode for low latency

  // Wake the sensor from sleep.
  if (!writeRegister(kRegPwrMgmt1, 0x00)) {
    Serial.printf("[%s] Failed to wake MPU-6050. Check wiring.\n", label_);
    return false;
  }

  // Configure accelerometer for ±2 g range (highest precision).
  if (!writeRegister(kRegAccelConfig, 0x00)) {
    Serial.printf("[%s] Failed to set accelerometer range.\n", label_);
    return false;
  }

  // Configure gyroscope for ±2000 °/s to handle fast head movements.
  if (!writeRegister(kRegGyroConfig, 0x18)) {
    Serial.printf("[%s] Failed to set gyroscope range.\n", label_);
    return false;
  }

  filter_.reset();
  Serial.printf("[%s] MPU-6050 initialized.\n", label_);
  return true;
}

bool ImuMpu6050::writeRegister(uint8_t reg, uint8_t value) {
  bus_->beginTransmission(address_);
  bus_->write(reg);
  bus_->write(value);
  return bus_->endTransmission() == 0;
}

bool ImuMpu6050::readRegisters(uint8_t start_reg, uint8_t* buffer, size_t length) {
  bus_->beginTransmission(address_);
  bus_->write(start_reg);
  if (bus_->endTransmission(false) != 0) {
    return false;
  }
  bus_->requestFrom(static_cast<int>(address_), static_cast<int>(length));
  size_t index = 0;
  while (bus_->available() && index < length) {
    buffer[index++] = static_cast<uint8_t>(bus_->read());
  }
  return index == length;
}

bool ImuMpu6050::read(float* gx, float* gy, float* gz, float* ax, float* ay, float* az) {
  uint8_t buffer[14];
  if (!readRegisters(kRegAccelStart, buffer, sizeof(buffer))) {
    Serial.printf("[%s] Failed to read sensor block.\n", label_);
    return false;
  }

  const int16_t accel_x = (buffer[0] << 8) | buffer[1];
  const int16_t accel_y = (buffer[2] << 8) | buffer[3];
  const int16_t accel_z = (buffer[4] << 8) | buffer[5];
  const int16_t gyro_x = (buffer[8] << 8) | buffer[9];
  const int16_t gyro_y = (buffer[10] << 8) | buffer[11];
  const int16_t gyro_z = (buffer[12] << 8) | buffer[13];

  // Convert to SI units and radians per second.
  *ax = static_cast<float>(accel_x) * accel_scale_;
  *ay = static_cast<float>(accel_y) * accel_scale_;
  *az = static_cast<float>(accel_z) * accel_scale_;
  *gx = deg2rad(static_cast<float>(gyro_x) * gyro_scale_);
  *gy = deg2rad(static_cast<float>(gyro_y) * gyro_scale_);
  *gz = deg2rad(static_cast<float>(gyro_z) * gyro_scale_);
  return true;
}

bool ImuMpu6050::update(float delta_seconds) {
  float gx, gy, gz, ax, ay, az;
  if (!read(&gx, &gy, &gz, &ax, &ay, &az)) {
    return false;
  }
  filter_.update(gx, gy, gz, ax, ay, az, delta_seconds);
  return true;
}

