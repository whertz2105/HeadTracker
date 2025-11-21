#pragma once

#include <Arduino.h>

#include "util.h"

// -----------------------------------------------------------------------------
// Radio Packet Format
// -----------------------------------------------------------------------------
// The headset sends 9-byte packets to the drone at ~30 Hz. Each field is heavily
// commented to demystify the binary layout for newcomers.
// -----------------------------------------------------------------------------

struct HeadsetPacket {
  uint8_t flags = 0;      // Future use (bit0 = recenter pressed)
  uint8_t sequence = 0;   // Increments every packet so the drone can detect drops
  int16_t yaw_cd = 0;     // Head yaw in centidegrees (0.01° units)
  int16_t pitch_cd = 0;   // Head pitch in centidegrees
  int16_t roll_cd = 0;    // Head roll in centidegrees
  uint8_t crc = 0;        // CRC-8 checksum over bytes 0..7
};

constexpr size_t kPacketSize = sizeof(HeadsetPacket);

// Encode Euler angles (radians) into a packet.
inline HeadsetPacket makePacket(const EulerAngles& angles, bool recenter_pressed,
                                uint8_t sequence) {
  HeadsetPacket pkt;
  pkt.flags = recenter_pressed ? 0x01 : 0x00;
  pkt.sequence = sequence;
  pkt.yaw_cd = static_cast<int16_t>(rad2deg(angles.yaw) * 100.0f);
  pkt.pitch_cd = static_cast<int16_t>(rad2deg(angles.pitch) * 100.0f);
  pkt.roll_cd = static_cast<int16_t>(rad2deg(angles.roll) * 100.0f);

  // Calculate CRC over the first 8 bytes. We temporarily treat the struct as a
  // byte buffer, which is safe because it only contains fixed-width types.
  pkt.crc = crc8(reinterpret_cast<const uint8_t*>(&pkt), kPacketSize - 1);
  return pkt;
}

// Validate a received packet, returning true if the checksum matches.
inline bool validatePacket(const HeadsetPacket& pkt) {
  const uint8_t calc = crc8(reinterpret_cast<const uint8_t*>(&pkt), kPacketSize - 1);
  return calc == pkt.crc;
}

// Convert centidegree fields back to radians for control math.
inline EulerAngles packetToEuler(const HeadsetPacket& pkt) {
  EulerAngles eul;
  eul.yaw = deg2rad(static_cast<float>(pkt.yaw_cd) / 100.0f);
  eul.pitch = deg2rad(static_cast<float>(pkt.pitch_cd) / 100.0f);
  eul.roll = deg2rad(static_cast<float>(pkt.roll_cd) / 100.0f);
  return eul;
}

