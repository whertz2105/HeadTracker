#pragma once

// -----------------------------------------------------------------------------
// Transport Peer Configuration
// -----------------------------------------------------------------------------
// Each ESP32 must know the MAC address of its counterpart when ESP-NOW is used.
// The easiest way to discover these addresses is to flash the helper sketch in
// firmware/tools/show_mac/ (see README for full instructions) and read the
// printed value from the serial monitor.  Paste the drone's MAC into the
// HEADSET_SIDE section and the headset's MAC into the DRONE_SIDE section below.
//
// Example format printed by WiFi.macAddress(): "24:6F:28:AA:BB:CC".  Replace the
// hexadecimal pairs below with your exact values.  Do not include braces or
// commas when copying the text from the serial monitor.
// -----------------------------------------------------------------------------

#include <cstdint>

#if defined(HEADSET_SIDE)
static const uint8_t PEER_MAC[6] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};  // Drone MAC
#elif defined(DRONE_SIDE)
static const uint8_t PEER_MAC[6] = {0x24, 0x6F, 0x28, 0xDD, 0xEE, 0xFF};  // Headset MAC
#else
#  error "Define HEADSET_SIDE or DRONE_SIDE in build_flags"
#endif

