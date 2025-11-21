#if defined(TRANSPORT_HC12)

#include <Arduino.h>

#include "transport.h"
#include "util.h"

namespace {
RxHandler g_on_rx = nullptr;

// Small ring buffer so we can group bytes before handing them to the
// application.  The HC-12 is transparent UART so data arrives byte by byte.
constexpr size_t kBufferSize = 64;
uint8_t g_buffer[kBufferSize];
size_t g_count = 0;

void flushBuffer() {
  if (g_on_rx && g_count > 0) {
    g_on_rx(g_buffer, g_count);
  }
  g_count = 0;
}

}  // namespace

namespace Transport {

bool begin(RxHandler onRx) {
  g_on_rx = onRx;

  pinMode(Pins::kHc12Set, OUTPUT);
  digitalWrite(Pins::kHc12Set, HIGH);  // Normal transparent mode.
  Serial2.begin(9600, SERIAL_8N1, Pins::kHc12Rx, Pins::kHc12Tx);
  Serial.println(F("[Transport] HC-12 serial ready."));
  return true;
}

bool send(const uint8_t* data, size_t len) {
  if (!data || len == 0) {
    return false;
  }
  size_t written = Serial2.write(data, len);
  if (written != len) {
    Serial.println(F("[Transport] HC-12 write incomplete."));
    return false;
  }
  return true;
}

void loop() {
  while (Serial2.available() > 0) {
    uint8_t b = static_cast<uint8_t>(Serial2.read());
    if (g_count < kBufferSize) {
      g_buffer[g_count++] = b;
    } else {
      flushBuffer();
      g_buffer[g_count++] = b;
    }
  }

  // Deliver any accumulated bytes each pass so the application can parse them.
  if (g_count > 0) {
    flushBuffer();
  }
}

}  // namespace Transport

#endif  // defined(TRANSPORT_HC12)

