#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "madgwick.h"
#include "util.h"

// -----------------------------------------------------------------------------
// MPU-6050 Helper Class
// -----------------------------------------------------------------------------
// Provides easy-to-read methods for initializing the sensor, reading gyro/accel
// data, and running the Madgwick filter. Each method prints clear error messages
// so a beginner knows exactly what went wrong.
// -----------------------------------------------------------------------------

class ImuMpu6050 {
 public:
  // Construct the helper with a human-friendly name (e.g., "Headset"). The name
  // appears in log messages so you can tell which board is speaking.
  explicit ImuMpu6050(const char* label);

  // Initialize the I²C bus and the MPU-6050 registers. Returns true on success.
  bool begin(TwoWire& bus = Wire, uint8_t address = 0x68);

  // Perform a sensor read, returning raw gyro (rad/s) and accel (m/s^2) values.
  bool read(float* gx, float* gy, float* gz, float* ax, float* ay, float* az);

  // Update the Madgwick filter using the latest sensor data. The update rate is
  // determined by `delta_seconds`, which you compute from `micros()` timestamps.
  bool update(float delta_seconds);

  // Retrieve the latest orientation quaternion.
  Quaternion orientation() const { return filter_.quaternion(); }

  // Expose the filter for advanced configuration (e.g., adjusting beta).
  MadgwickFilter& filter() { return filter_; }

 private:
  bool writeRegister(uint8_t reg, uint8_t value);
  bool readRegisters(uint8_t start_reg, uint8_t* buffer, size_t length);

  const char* label_;
  TwoWire* bus_ = nullptr;
  uint8_t address_ = 0x68;
  MadgwickFilter filter_;
  float gyro_scale_ = 2000.0f / 32768.0f;    // deg/s per LSB for ±2000 dps
  float accel_scale_ = 9.80665f / 16384.0f;  // m/s^2 per LSB for ±2 g
};

