#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <functional>

// Wi-Fi station + a small REST API (built-in synchronous WebServer, no
// extra library dependency) mirroring the BLE control protocol.
class NetManager {
public:
  using ControlCallback = std::function<void(const String &json)>;
  using StatusProvider = std::function<String()>;

  void begin(const char *ssid, const char *password,
             uint32_t connectTimeoutMs, StatusProvider statusProvider,
             ControlCallback onControl);
  void loop();
  bool isConnected() const;

private:
  WebServer server_{80};
  StatusProvider statusProvider_;
  ControlCallback onControl_;
};
