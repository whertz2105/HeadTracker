#include "madgwick.h"

// -----------------------------------------------------------------------------
// Implementation of the Madgwick AHRS filter. The math is based on the original
// 2010 paper by Sebastian Madgwick but annotated here for readability.
// -----------------------------------------------------------------------------

MadgwickFilter::MadgwickFilter(float beta) : beta_(beta) { reset(); }

void MadgwickFilter::reset() { q_ = Quaternion{1.0f, 0.0f, 0.0f, 0.0f}; }

void MadgwickFilter::update(float gx, float gy, float gz, float ax, float ay, float az,
                            float delta_seconds) {
  // Normalize the accelerometer measurement so we only use its direction (gravity).
  const float accel_norm = sqrtf(ax * ax + ay * ay + az * az);
  if (accel_norm < 1e-6f) {
    return;  // Avoid division by zero if the sensor reports near-zero magnitude.
  }
  ax /= accel_norm;
  ay /= accel_norm;
  az /= accel_norm;

  // Short names for readability.
  float qw = q_.w;
  float qx = q_.x;
  float qy = q_.y;
  float qz = q_.z;

  // Reference direction of Earth's gravitational field.
  const float f1 = 2.0f * (qx * qz - qw * qy) - ax;
  const float f2 = 2.0f * (qw * qx + qy * qz) - ay;
  const float f3 = 2.0f * (0.5f - qx * qx - qy * qy) - az;

  // Gradient descent corrective step.
  const float grad_w = 2.0f * (-qy * f1 + qx * f2);
  const float grad_x = 2.0f * (qz * f1 + qw * f2 - 2.0f * qx * f3);
  const float grad_y = 2.0f * (-qw * f1 + qz * f2 - 2.0f * qy * f3);
  const float grad_z = 2.0f * (qx * f1 + qy * f2);

  // Normalize gradient to maintain consistent step size.
  const float grad_norm = sqrtf(grad_w * grad_w + grad_x * grad_x + grad_y * grad_y +
                                grad_z * grad_z);
  float s_w = 0.0f, s_x = 0.0f, s_y = 0.0f, s_z = 0.0f;
  if (grad_norm > 1e-6f) {
    s_w = grad_w / grad_norm;
    s_x = grad_x / grad_norm;
    s_y = grad_y / grad_norm;
    s_z = grad_z / grad_norm;
  }

  // Integrate rate of change of quaternion.
  const float qw_dot = 0.5f * (-qx * gx - qy * gy - qz * gz) - beta_ * s_w;
  const float qx_dot = 0.5f * (qw * gx + qy * gz - qz * gy) - beta_ * s_x;
  const float qy_dot = 0.5f * (qw * gy - qx * gz + qz * gx) - beta_ * s_y;
  const float qz_dot = 0.5f * (qw * gz + qx * gy - qy * gx) - beta_ * s_z;

  qw += qw_dot * delta_seconds;
  qx += qx_dot * delta_seconds;
  qy += qy_dot * delta_seconds;
  qz += qz_dot * delta_seconds;

  q_ = Quaternion{qw, qx, qy, qz};
  quatNormalize(&q_);
}

