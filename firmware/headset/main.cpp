#include <Arduino.h>
#include <Wire.h>

#include "../common/imu_mpu6050.h"
#include "../common/packet.h"
#include "../common/transport.h"
#include "../common/transport_config.h"
#include "../common/util.h"

#if defined(TRANSPORT_ESP_NOW)
#  include "../common/transport_espnow.cpp"
#elif defined(TRANSPORT_HC12)
#  include "../common/transport_hc12.cpp"
#else
#  error "Select TRANSPORT_ESP_NOW or TRANSPORT_HC12 in build_flags"
#endif

// -----------------------------------------------------------------------------
// Headset Firmware Entry Point
// -----------------------------------------------------------------------------
// Responsibilities:
//   * Read the MPU-6050 IMU at high rate and fuse data with Madgwick filter.
//   * Transmit yaw/pitch/roll quaternions to the drone via the selected transport.
//   * Provide a recenter button so the pilot can reset yaw alignment.
//   * Offer a friendly serial console for diagnostics and calibration.
// -----------------------------------------------------------------------------

ImuMpu6050 imu("Headset");                   // IMU helper with descriptive name
Quaternion referenceOffset{1, 0, 0, 0};       // Stores recenter offset
uint8_t sequence_counter = 0;                 // Packet sequence number
uint32_t last_broadcast_ms = 0;               // Track packet rate
uint32_t last_filter_update_us = 0;           // For computing delta time

// Forward declaration for readability.
void processSerialCommand(int ch);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("[Headset] Booting..."));

  pinMode(Pins::kStatusLed, OUTPUT);
  pinMode(Pins::kRecenterButton, INPUT_PULLUP);

  // Bring up I²C and IMU.
  if (!imu.begin(Wire)) {
    Serial.println(F("[Headset] IMU init failed. Check wiring."));
    blinkStatusLed(100, 100, 10);
  }

  // Bring up the selected transport (ESP-NOW for bench tests or HC-12 for
  // long-range flying).  Beginners can inspect the serial log to confirm the
  // transport printed a success message.
  if (!Transport::begin(nullptr)) {
    Serial.println(F("[Headset] Transport init failed. Check settings."));
  }

  last_filter_update_us = micros();
  Serial.println(F("[Headset] Ready."));
}

void loop() {
  const uint32_t now_us = micros();
  float delta_seconds = (now_us - last_filter_update_us) / 1e6f;
  if (delta_seconds <= 0.0f || delta_seconds > 0.1f) {
    // Protect against micros() wraparound or long pauses by clamping dt.
    delta_seconds = 0.005f;
  }
  last_filter_update_us = now_us;

  // Service the transport in case it needs polling (UART).
  Transport::loop();

  // Update orientation estimate.
  if (!imu.update(delta_seconds)) {
    // Error already logged by helper; blink LED rapidly to warn the pilot.
    blinkStatusLed(50, 50, 2);
    return;
  }

  // Apply recenter offset if the button is pressed.
  if (digitalRead(Pins::kRecenterButton) == LOW) {
    referenceOffset = quatConjugate(imu.orientation());
    Serial.println(F("[Headset] Recentered orientation."));
    setStatusLed(true);
  } else {
    setStatusLed(false);
  }

  // Compute final orientation.
  Quaternion q = quatMultiply(referenceOffset, imu.orientation());
  quatNormalize(&q);
  EulerAngles angles = quatToEuler(q);

  // Prepare and send packet every ~33 ms (~30 Hz).
  const uint32_t now_ms = millis();
  if (now_ms - last_broadcast_ms >= 33) {
    last_broadcast_ms = now_ms;
    const bool recenter_pressed = digitalRead(Pins::kRecenterButton) == LOW;
    HeadsetPacket pkt = makePacket(angles, recenter_pressed, sequence_counter++);
    // Send the packet through the transport layer (ESP-NOW or HC-12). We report
    // failures so beginners can spot wiring or pairing issues quickly.
    if (!Transport::send(reinterpret_cast<const uint8_t*>(&pkt), kPacketSize)) {
      Serial.println(F("[Headset] Transport send failed."));
    }
    toggleStatusLed();

    // Optional debug print for beginners to see values.
    Serial.printf(
        "[Headset] Seq %u Yaw %.1f Pitch %.1f Roll %.1f\n", pkt.sequence,
        static_cast<float>(pkt.yaw_cd) / 100.0f,
        static_cast<float>(pkt.pitch_cd) / 100.0f,
        static_cast<float>(pkt.roll_cd) / 100.0f);
  }

  // Handle serial commands when the user types in the monitor.
  while (Serial.available()) {
    processSerialCommand(Serial.read());
  }
}

// -----------------------------------------------------------------------------
// Serial Command Handler
// -----------------------------------------------------------------------------
void processSerialCommand(int ch) {
  switch (ch) {
    case 'o': {  // Print orientation
      Quaternion q = quatMultiply(referenceOffset, imu.orientation());
      quatNormalize(&q);
      EulerAngles e = quatToEuler(q);
      Serial.printf("[Headset] Orientation YPR (deg): %.2f %.2f %.2f\n", rad2deg(e.yaw),
                    rad2deg(e.pitch), rad2deg(e.roll));
      break;
    }
    case 'r': {
      float gx, gy, gz, ax, ay, az;
      if (imu.read(&gx, &gy, &gz, &ax, &ay, &az)) {
        Serial.printf("[Headset] Raw gyro (rad/s): %.3f %.3f %.3f | accel (m/s^2): %.3f %.3f %.3f\n",
                      gx, gy, gz, ax, ay, az);
      }
      break;
    }
    case 'c': {
      referenceOffset = quatConjugate(imu.orientation());
      Serial.println(F("[Headset] Manual recenter complete."));
      break;
    }
    default:
      Serial.println(F("Commands: o=orientation r=raw c=recenter"));
      break;
  }
}

