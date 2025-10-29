#pragma once

#include <Arduino.h>

#include "util.h"

// -----------------------------------------------------------------------------
// Madgwick Filter Interface
// -----------------------------------------------------------------------------
// This class wraps the Madgwick AHRS algorithm, providing a friendly Arduino-
// style API with clear comments describing each step. The implementation lives
// in `madgwick.cpp` and is tuned for 500 Hz sensor updates and 200 Hz filter
// updates.
// -----------------------------------------------------------------------------

class MadgwickFilter {
 public:
  // Create a filter with the specified beta gain (convergence rate). A value of
  // 0.1–0.2 works well for human motion tracking.
  explicit MadgwickFilter(float beta = 0.12f);

  // Reset the orientation to the identity quaternion (no rotation).
  void reset();

  // Update the orientation estimate given gyroscope (rad/s) and accelerometer
  // (m/s^2) measurements plus the elapsed time in seconds.
  void update(float gx, float gy, float gz, float ax, float ay, float az,
              float delta_seconds);

  // Fetch the current quaternion estimate.
  Quaternion quaternion() const { return q_; }

 private:
  float beta_;       // Filter gain controlling accelerometer influence
  Quaternion q_;     // Current orientation estimate
};

