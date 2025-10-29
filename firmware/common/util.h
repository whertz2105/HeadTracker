#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Utility helpers shared by both firmware targets. Everything is documented in
// plain English so newcomers understand what each function does and why it
// matters. Most helpers are implemented as inline functions for simplicity.
// -----------------------------------------------------------------------------

// This constant defines how long (in milliseconds) we consider a packet "fresh"
// before triggering the failsafe logic.
constexpr uint32_t kPacketTimeoutMs = 150;

// GPIO assignments are gathered here so they can be changed in one place if you
// wire your hardware differently. These match the defaults described in the docs.
namespace Pins {
constexpr uint8_t kI2CSda = 21;
constexpr uint8_t kI2CScl = 22;
constexpr uint8_t kHc12Tx = 17;
constexpr uint8_t kHc12Rx = 16;
constexpr uint8_t kHc12Set = 4;
constexpr uint8_t kStatusLed = 2;
constexpr uint8_t kRecenterButton = 0;  // Headset only; unused on drone
constexpr uint8_t kServoPan = 25;
constexpr uint8_t kServoTilt = 26;
constexpr uint8_t kPwmInput = 27;
}  // namespace Pins

// Simple struct to hold yaw/pitch/roll in radians. Keeping units consistent
// prevents confusion later when mapping to degrees or servo pulses.
struct EulerAngles {
  float yaw = 0.0f;   // Rotation around Z
  float pitch = 0.0f; // Rotation around Y
  float roll = 0.0f;  // Rotation around X
};

// This function reads the current time in milliseconds and casts it to a
// floating-point value. We use floating-point time when implementing filters so
// that the math is intuitive (seconds) rather than integer ticks.
inline float millisF() {
  return static_cast<float>(millis());
}

// Compute a CRC-8 checksum using the Dallas/Maxim polynomial (0x31). This small
// checksum fits in a single byte and catches most packet corruption issues.
inline uint8_t crc8(const uint8_t* data, size_t length) {
  uint8_t crc = 0x00;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

// Clamp a floating-point number between minimum and maximum bounds.
inline float clampf(float value, float min_value, float max_value) {
  return fminf(fmaxf(value, min_value), max_value);
}

// Linearly interpolate between two values, often used for smoothing transitions.
inline float lerpf(float start, float end, float alpha) {
  return start + (end - start) * alpha;
}

// Convert degrees to radians. Servos typically work in degrees, while trigonetic
// functions in C++ expect radians.
inline float deg2rad(float deg) { return deg * static_cast<float>(PI / 180.0); }

// Convert radians to degrees for human-readable logging.
inline float rad2deg(float rad) { return rad * static_cast<float>(180.0 / PI); }

// Structure representing a quaternion. The Madgwick filter populates this.
struct Quaternion {
  float w = 1.0f;
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

// Multiply two quaternions. This is the building block for combining rotations.
inline Quaternion quatMultiply(const Quaternion& a, const Quaternion& b) {
  Quaternion result;
  result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
  result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
  result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
  result.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
  return result;
}

// Return the conjugate of a quaternion (invert the vector part). This is used to
// "subtract" one orientation from another.
inline Quaternion quatConjugate(const Quaternion& q) {
  return Quaternion{q.w, -q.x, -q.y, -q.z};
}

// Normalize a quaternion to unit length. This prevents numerical drift from the
// sensor fusion algorithm.
inline void quatNormalize(Quaternion* q) {
  const float norm = sqrtf(q->w * q->w + q->x * q->x + q->y * q->y + q->z * q->z);
  if (norm > 0.0f) {
    q->w /= norm;
    q->x /= norm;
    q->y /= norm;
    q->z /= norm;
  }
}

// Convert a quaternion to Euler angles using the aerospace ZYX convention.

// Convert Euler angles (ZYX convention) to a quaternion.
inline Quaternion eulerToQuat(float yaw, float pitch, float roll) {
  const float cy = cosf(yaw * 0.5f);
  const float sy = sinf(yaw * 0.5f);
  const float cp = cosf(pitch * 0.5f);
  const float sp = sinf(pitch * 0.5f);
  const float cr = cosf(roll * 0.5f);
  const float sr = sinf(roll * 0.5f);
  Quaternion q;
  q.w = cr * cp * cy + sr * sp * sy;
  q.x = sr * cp * cy - cr * sp * sy;
  q.y = cr * sp * cy + sr * cp * sy;
  q.z = cr * cp * sy - sr * sp * cy;
  return q;
}
inline EulerAngles quatToEuler(const Quaternion& q) {
  EulerAngles angles;
  const float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
  const float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
  angles.roll = atan2f(sinr_cosp, cosr_cosp);

  const float sinp = 2.0f * (q.w * q.y - q.z * q.x);
  if (fabsf(sinp) >= 1.0f) {
    angles.pitch = copysignf(HALF_PI, sinp);
  } else {
    angles.pitch = asinf(sinp);
  }

  const float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
  const float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
  angles.yaw = atan2f(siny_cosp, cosy_cosp);
  return angles;
}

// Blend two angles while accounting for wraparound at ±π. This avoids the camera
// taking the long way around when slewing between angles.
inline float blendAngle(float current, float target, float max_delta) {
  float delta = target - current;
  while (delta > PI) delta -= TWO_PI;
  while (delta < -PI) delta += TWO_PI;
  delta = clampf(delta, -max_delta, max_delta);
  return current + delta;
}

// Simple LED helper to indicate status with blink patterns.
inline void setStatusLed(bool on) { digitalWrite(Pins::kStatusLed, on ? HIGH : LOW); }

inline void toggleStatusLed() { digitalWrite(Pins::kStatusLed, !digitalRead(Pins::kStatusLed)); }

// Blink the LED with a specific pattern. Duration is expressed in milliseconds.
inline void blinkStatusLed(uint16_t on_ms, uint16_t off_ms, uint8_t repeat = 1) {
  for (uint8_t i = 0; i < repeat; ++i) {
    setStatusLed(true);
    delay(on_ms);
    setStatusLed(false);
    delay(off_ms);
  }
}

