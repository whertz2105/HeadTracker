#include <WiFi.h>

// Simple helper sketch that prints the ESP32's station MAC address so you can
// paste it into transport_config.h when configuring ESP-NOW bench testing.
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print(F("Station MAC: "));
  Serial.println(WiFi.macAddress());
}

void loop() {
  // Nothing to do here.  Leave the board powered so you can read the serial
  // output.  The MAC address remains constant across reboots for the same board.
}

