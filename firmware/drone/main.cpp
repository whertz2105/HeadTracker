#include <Arduino.h>
#include <Wire.h>

#include "../common/imu_mpu6050.h"
#include "../common/packet.h"
#include "../common/servo_map.h"
#include "../common/util.h"

#include <cstring>

// -----------------------------------------------------------------------------
// Drone Firmware Entry Point
// -----------------------------------------------------------------------------
// Responsibilities:
//   * Read the drone-mounted MPU-6050 to know the aircraft orientation.
//   * Receive headset packets via HC-12 and combine with drone orientation.
//   * Interpret the transmitter's mode switch from a PWM input.
//   * Command pan/tilt servos with low-pass filtering, slew limiting, and failsafe.
// -----------------------------------------------------------------------------

ImuMpu6050 imu("Drone");

ServoConfig pan_servo{Pins::kServoPan, 0, 1000, 2000, -120.0f, 120.0f};
ServoConfig tilt_servo{Pins::kServoTilt, 1, 1000, 2000, -90.0f, 45.0f};

EulerAngles head_angles_world{};  // Updated when packets arrive
bool packet_valid = false;
uint32_t last_packet_ms = 0;
uint8_t last_sequence = 0;

float filtered_pan_deg = 0.0f;
float filtered_tilt_deg = 0.0f;
float slew_pan_deg = 0.0f;
float slew_tilt_deg = 0.0f;

enum class Mode { OnlyHorizontal, FreeMove, Bombing };

// Helper prototypes
void processSerialCommand(int ch);
Mode readModeSwitch();
float readPwmMicroseconds();
void applyFailsafe(float dt_ms, float* target_pan, float* target_tilt);
void updateServos(float dt_ms, float target_pan_deg, float target_tilt_deg);
void handlePacket(const HeadsetPacket& pkt);
void pollHc12();

namespace {
uint8_t rx_buffer[64];
size_t rx_count = 0;
}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("[Drone] Booting..."));

  pinMode(Pins::kStatusLed, OUTPUT);
  pinMode(Pins::kPwmInput, INPUT);

  if (!imu.begin(Wire)) {
    Serial.println(F("[Drone] IMU init failed."));
    blinkStatusLed(100, 100, 10);
  }

  // Configure the HC-12 serial radio for transparent operation.
  pinMode(Pins::kHc12Set, OUTPUT);
  digitalWrite(Pins::kHc12Set, HIGH);
  Serial2.begin(9600, SERIAL_8N1, Pins::kHc12Rx, Pins::kHc12Tx);
  Serial.println(F("[Drone] HC-12 serial ready."));

  // Configure hardware PWM for servos.
  ledcSetup(pan_servo.ledc_channel, 50, 16);
  ledcAttachPin(pan_servo.pin, pan_servo.ledc_channel);
  ledcSetup(tilt_servo.ledc_channel, 50, 16);
  ledcAttachPin(tilt_servo.pin, tilt_servo.ledc_channel);

  filtered_pan_deg = 0.0f;
  filtered_tilt_deg = 0.0f;
  slew_pan_deg = 0.0f;
  slew_tilt_deg = 0.0f;

  Serial.println(F("[Drone] Ready."));
}

void loop() {
  // Service the HC-12 serial stream and decode any complete packets that
  // arrived since the previous loop iteration.
  pollHc12();

  const uint32_t now_us = micros();
  static uint32_t last_update_us = now_us;
  float dt_seconds = (now_us - last_update_us) / 1e6f;
  if (dt_seconds <= 0.0f || dt_seconds > 0.1f) {
    dt_seconds = 0.005f;
  }
  last_update_us = now_us;
  const float dt_ms = dt_seconds * 1000.0f;

  if (!imu.update(dt_seconds)) {
    blinkStatusLed(50, 50, 2);
    return;
  }

  // Determine current mode from PWM input.
  Mode mode = readModeSwitch();

  // Calculate target angles based on mode.
  Quaternion q_drone = imu.orientation();
  quatNormalize(&q_drone);
  EulerAngles drone_euler = quatToEuler(q_drone);

  float target_pan_deg = 0.0f;
  float target_tilt_deg = 0.0f;

  if (packet_valid) {
    Quaternion q_head_world = eulerToQuat(head_angles_world.yaw, head_angles_world.pitch,
                                          head_angles_world.roll);
    Quaternion q_rel = quatMultiply(quatConjugate(q_drone), q_head_world);
    quatNormalize(&q_rel);
    EulerAngles rel = quatToEuler(q_rel);

    switch (mode) {
      case Mode::OnlyHorizontal: {
        target_pan_deg = rad2deg(rel.yaw);
        const float tilt = rad2deg(head_angles_world.pitch - drone_euler.pitch);
        target_tilt_deg = clampf(tilt, -90.0f, 30.0f);
        break;
      }
      case Mode::FreeMove: {
        target_pan_deg = rad2deg(rel.yaw);
        target_tilt_deg = clampf(rad2deg(rel.pitch), -90.0f, 30.0f);
        break;
      }
      case Mode::Bombing: {
        target_pan_deg = 0.0f;
        target_tilt_deg = -90.0f;
        break;
      }
    }
  } else {
    target_pan_deg = 0.0f;
    target_tilt_deg = 0.0f;
  }

  // Apply failsafe if no recent packets.
  applyFailsafe(dt_ms, &target_pan_deg, &target_tilt_deg);

  // Low-pass filter to soften motion (5 Hz cutoff).
  const float cutoff_hz = 5.0f;
  const float rc = 1.0f / (2.0f * PI * cutoff_hz);
  const float alpha = dt_seconds / (rc + dt_seconds);
  filtered_pan_deg = filtered_pan_deg + alpha * (target_pan_deg - filtered_pan_deg);
  filtered_tilt_deg = filtered_tilt_deg + alpha * (target_tilt_deg - filtered_tilt_deg);

  updateServos(dt_ms, filtered_pan_deg, filtered_tilt_deg);
  static uint32_t last_status_ms = 0;
  if (millis() - last_status_ms > 100) {
    last_status_ms = millis();
    const char* mode_name = (mode == Mode::Bombing) ? "Bombing" : (mode == Mode::FreeMove ? "FreeMove" : "OnlyHorizontal");
    Serial.printf("[Drone] Status %s pan %.1f tilt %.1f pwm %.0f\n", mode_name, slew_pan_deg, slew_tilt_deg, readPwmMicroseconds());
  }

  // Process console commands.
  while (Serial.available()) {
    processSerialCommand(Serial.read());
  }
}

void pollHc12() {
  // Read all available bytes from Serial2, placing them into a small FIFO. The
  // headset sends fixed-length packets, so once the FIFO holds nine bytes we
  // can decode and process the message.
  while (Serial2.available() > 0) {
    uint8_t byte_in = static_cast<uint8_t>(Serial2.read());
    if (rx_count < sizeof(rx_buffer)) {
      rx_buffer[rx_count++] = byte_in;
    } else {
      // If the buffer overflows we reset it so parsing can resynchronize.
      rx_count = 0;
      rx_buffer[rx_count++] = byte_in;
    }
  }

  while (rx_count >= kPacketSize) {
    HeadsetPacket pkt;
    std::memcpy(&pkt, rx_buffer, kPacketSize);

    if (validatePacket(pkt)) {
      rx_count -= kPacketSize;
      if (rx_count > 0) {
        std::memmove(rx_buffer, rx_buffer + kPacketSize, rx_count);
      }
      handlePacket(pkt);
    } else {
      Serial.println(F("[Drone] CRC mismatch. Resyncing..."));
      // Drop the oldest byte and try again next iteration to regain alignment.
      rx_count -= 1;
      if (rx_count > 0) {
        std::memmove(rx_buffer, rx_buffer + 1, rx_count);
      }
    }
  }
}

void handlePacket(const HeadsetPacket& pkt) {
  packet_valid = true;
  last_packet_ms = millis();
  head_angles_world = packetToEuler(pkt);
  last_sequence = pkt.sequence;
  Serial.printf("[Drone] Packet seq %u yaw %.1f pitch %.1f roll %.1f\n", pkt.sequence,
                rad2deg(head_angles_world.yaw), rad2deg(head_angles_world.pitch),
                rad2deg(head_angles_world.roll));
  toggleStatusLed();
}

// -----------------------------------------------------------------------------
// Mode Reading and Failsafe Helpers
// -----------------------------------------------------------------------------
Mode readModeSwitch() {
  const float pwm = readPwmMicroseconds();
  if (pwm > 1700.0f) {
    return Mode::Bombing;
  } else if (pwm > 1300.0f) {
    return Mode::FreeMove;
  }
  return Mode::OnlyHorizontal;
}

float readPwmMicroseconds() {
  // Use pulseIn with a short timeout. It blocks up to 25 ms but the call is only
  // made once per loop, keeping latency acceptable for 30 Hz updates.
  unsigned long high = pulseIn(Pins::kPwmInput, HIGH, 25000);
  if (high == 0) {
    return 1500.0f;  // Default to middle position if no signal detected.
  }
  return static_cast<float>(high);
}

void applyFailsafe(float dt_ms, float* target_pan, float* target_tilt) {
  const uint32_t now_ms = millis();
  if (packet_valid && (now_ms - last_packet_ms) <= kPacketTimeoutMs) {
    return;  // Packets fresh; nothing to do.
  }

  // Begin blending back to neutral over ~0.75 s.
  const float blend_rate = dt_ms / 750.0f;
  *target_pan = lerpf(*target_pan, 0.0f, clampf(blend_rate, 0.0f, 1.0f));
  *target_tilt = lerpf(*target_tilt, 0.0f, clampf(blend_rate, 0.0f, 1.0f));
}

void updateServos(float dt_ms, float target_pan_deg, float target_tilt_deg) {
  // Apply slew limits.
  slew_pan_deg = applySlewLimit(slew_pan_deg, target_pan_deg, 120.0f, dt_ms);
  slew_tilt_deg = applySlewLimit(slew_tilt_deg, target_tilt_deg, 90.0f, dt_ms);

  writeServoAngle(pan_servo, slew_pan_deg);
  writeServoAngle(tilt_servo, slew_tilt_deg);
}

// -----------------------------------------------------------------------------
// Serial Command Handler
// -----------------------------------------------------------------------------
void processSerialCommand(int ch) {
  switch (ch) {
    case 'o': {
      EulerAngles e = quatToEuler(imu.orientation());
      Serial.printf("[Drone] Orientation YPR (deg): %.2f %.2f %.2f\n", rad2deg(e.yaw),
                    rad2deg(e.pitch), rad2deg(e.roll));
      break;
    }
    case 'p': {
      Serial.printf("[Drone] Last packet seq %u age %lu ms\n", last_sequence,
                    static_cast<unsigned long>(millis() - last_packet_ms));
      break;
    }
    case 'f': {
      packet_valid = false;
      Serial.println(F("[Drone] Failsafe forced."));
      break;
    }
    default:
      Serial.println(F("Commands: o=orientation p=packet f=failsafe"));
      break;
  }
}

