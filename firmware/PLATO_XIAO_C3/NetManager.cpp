#include "NetManager.h"

#include <WiFi.h>

void NetManager::begin(const char *ssid, const char *password,
                        uint32_t connectTimeoutMs,
                        StatusProvider statusProvider,
                        ControlCallback onControl) {
  statusProvider_ = statusProvider;
  onControl_ = onControl;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < connectTimeoutMs) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[NetManager] Wi-Fi connected, IP: %s\n",
                   WiFi.localIP().toString().c_str());
  } else {
    Serial.println(
        "[NetManager] Wi-Fi connect failed, continuing offline (BLE still available)");
  }

  server_.on("/api/status", HTTP_GET, [this]() {
    server_.send(200, "application/json",
                 statusProvider_ ? statusProvider_() : "{}");
  });

  server_.on("/api/control", HTTP_POST, [this]() {
    if (onControl_) {
      onControl_(server_.arg("plain"));
    }
    server_.send(200, "application/json", "{\"ok\":true}");
  });

  server_.begin();
}

void NetManager::loop() {
  if (WiFi.status() == WL_CONNECTED) {
    server_.handleClient();
  }
}

bool NetManager::isConnected() const { return WiFi.status() == WL_CONNECTED; }
