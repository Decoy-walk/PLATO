#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <functional>

// BLE GATT service mirroring the HTTP control protocol for local/offline
// control (e.g. from nRF Connect or a phone app) via NimBLE-Arduino.
class BleManager {
public:
  using ControlCallback = std::function<void(const String &json)>;

  void begin(const char *deviceName, const char *serviceUuid,
             const char *sensorCharUuid, const char *controlCharUuid,
             ControlCallback onControl);
  void notifySensor(const String &json);

private:
  class ControlCallbacks : public NimBLECharacteristicCallbacks {
  public:
    explicit ControlCallbacks(ControlCallback cb) : cb_(cb) {}
    void onWrite(NimBLECharacteristic *characteristic) override;

  private:
    ControlCallback cb_;
  };

  NimBLECharacteristic *sensorChar_ = nullptr;
  NimBLECharacteristic *controlChar_ = nullptr;
  ControlCallbacks *controlCallbacks_ = nullptr;
};
