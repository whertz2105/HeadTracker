#if defined(TRANSPORT_ESP_NOW)

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <cstring>

#include "transport.h"
#include "transport_config.h"

namespace {
RxHandler g_on_rx = nullptr;

// Callback invoked by the ESP-NOW stack whenever data arrives.
void onEspNowReceive(const uint8_t* mac, const uint8_t* data, int len) {
  (void)mac;  // Unused because we only talk to a single peer.
  if (g_on_rx && data && len > 0) {
    g_on_rx(data, static_cast<size_t>(len));
  }
}

// Optional callback to report send status so beginners can see feedback.
void onEspNowSent(const uint8_t* mac, esp_now_send_status_t status) {
  (void)mac;
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println(F("[Transport] ESP-NOW send failed."));
  }
}

}  // namespace

namespace Transport {

bool begin(RxHandler onRx) {
  g_on_rx = onRx;

  WiFi.mode(WIFI_STA);  // Required before starting ESP-NOW.
  if (esp_now_init() != ESP_OK) {
    Serial.println(F("[Transport] ESP-NOW init failed. Reset and retry."));
    return false;
  }

  esp_now_register_recv_cb(onEspNowReceive);
  esp_now_register_send_cb(onEspNowSent);

  esp_now_peer_info_t peer{};
  std::memset(&peer, 0, sizeof(peer));
  std::memcpy(peer.peer_addr, PEER_MAC, sizeof(PEER_MAC));
  peer.ifidx = WIFI_IF_STA;
  peer.channel = 0;  // Default channel works for bench testing.
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println(F("[Transport] Failed to add ESP-NOW peer."));
    return false;
  }

  Serial.println(F("[Transport] ESP-NOW init OK."));
  return true;
}

bool send(const uint8_t* data, size_t len) {
  if (!data || len == 0) {
    return false;
  }
  esp_err_t err = esp_now_send(PEER_MAC, data, len);
  if (err != ESP_OK) {
    Serial.printf("[Transport] ESP-NOW send error %d\n", static_cast<int>(err));
    return false;
  }
  return true;
}

void loop() {
  // ESP-NOW operates via interrupts, so nothing is required here.
}

}  // namespace Transport

#endif  // defined(TRANSPORT_ESP_NOW)

