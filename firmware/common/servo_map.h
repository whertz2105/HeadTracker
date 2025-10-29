#pragma once

#include <Arduino.h>

#include "util.h"

// -----------------------------------------------------------------------------
// Servo Mapping Helpers
// -----------------------------------------------------------------------------
// These functions convert between physical angles (degrees) and the pulse widths
// required by hobby servos. The defaults assume 1000–2000 µs limits but you can
// adjust them to match your servos if needed.
// -----------------------------------------------------------------------------

struct ServoConfig {
  uint8_t pin;            // GPIO pin number
  uint8_t ledc_channel;   // LEDC timer channel used by the ESP32 PWM hardware
  uint16_t pulse_min_us;  // Minimum pulse width in microseconds
  uint16_t pulse_max_us;  // Maximum pulse width in microseconds
  float angle_min_deg;    // Minimum angle supported by the servo linkage
  float angle_max_deg;    // Maximum angle supported by the servo linkage
};

// Convert an angle in degrees to a pulse width using a linear map.
inline uint16_t angleToPulseUs(const ServoConfig& cfg, float angle_deg) {
  angle_deg = clampf(angle_deg, cfg.angle_min_deg, cfg.angle_max_deg);
  const float span_deg = cfg.angle_max_deg - cfg.angle_min_deg;
  const float span_us = static_cast<float>(cfg.pulse_max_us - cfg.pulse_min_us);
  const float ratio = (angle_deg - cfg.angle_min_deg) / span_deg;
  return static_cast<uint16_t>(cfg.pulse_min_us + ratio * span_us);
}

// Convert PWM duty to the value expected by the ESP32 LEDC driver.
inline uint32_t pulseUsToDuty(uint16_t pulse_us, uint32_t pwm_freq_hz = 50) {
  // The ESP32 LEDC peripheral uses a 20-bit resolution counter by default.
  const uint32_t resolution_bits = 16;  // Use 16 bits to reduce CPU usage.
  const uint32_t max_duty = (1u << resolution_bits) - 1u;
  const float period_us = 1e6f / static_cast<float>(pwm_freq_hz);
  const float duty_ratio = static_cast<float>(pulse_us) / period_us;
  return static_cast<uint32_t>(duty_ratio * max_duty);
}

// Update the LEDC hardware with the requested angle.
inline void writeServoAngle(const ServoConfig& cfg, float angle_deg) {
  const uint16_t pulse = angleToPulseUs(cfg, angle_deg);
  const uint32_t duty = pulseUsToDuty(pulse);
  ledcWrite(cfg.ledc_channel, duty);
}

// Slew limiter that ensures the servo angle does not change faster than the
// specified degrees per second. dt_ms is the elapsed time since the last update.
inline float applySlewLimit(float current_deg, float target_deg, float limit_deg_per_s,
                            float dt_ms) {
  const float max_delta = limit_deg_per_s * (dt_ms / 1000.0f);
  return rad2deg(blendAngle(deg2rad(current_deg), deg2rad(target_deg), deg2rad(max_delta)));
}

