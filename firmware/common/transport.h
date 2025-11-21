#pragma once

// -----------------------------------------------------------------------------
// Transport Abstraction Layer
// -----------------------------------------------------------------------------
// The project supports multiple physical communication links (HC-12 serial radio
// for long range and ESP-NOW for short-range bench testing).  This header
// defines the small interface used by the application code so different
// transports can be swapped without touching the main logic files.
//
// Usage pattern:
//   * Call Transport::begin() during setup and provide a function that should
//     run whenever bytes are received.  The handler must be fast because it is
//     triggered from the transport driver context.
//   * Call Transport::send() whenever you have a packet ready.  Packets up to
//     64 bytes are recommended so they fit easily across all transports.
//   * Call Transport::loop() from your main loop so transports that require
//     polling (such as UART) can move bytes along.
// -----------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>
#include <functional>

// RxHandler is a callback type that receives raw byte buffers.  The handler is
// allowed to copy the data or process it immediately; the buffer is only valid
// during the callback.
using RxHandler = std::function<void(const uint8_t*, size_t)>;

namespace Transport {

// Initializes the chosen transport.  Returns true on success.  The onRx handler
// may be nullptr if the caller does not need to process incoming data.
bool begin(RxHandler onRx);

// Sends a block of bytes.  Returns true if the full buffer was accepted by the
// transport driver.
bool send(const uint8_t* data, size_t len);

// Called regularly from loop() so polling-based transports can process their
// buffers.  Transports that run purely on interrupts may leave this empty.
void loop();

}  // namespace Transport

