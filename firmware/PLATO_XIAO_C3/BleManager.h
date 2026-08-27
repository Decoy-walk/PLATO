#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>

// BLE GATT service that streams the fold-state bitmask as a compact binary
// notification for a PC-side logging bridge. Binary (not JSON) so the
// 20-hinge payload always fits a single ATT notification regardless of
// whether the central negotiated a larger MTU.
class BleManager {
public:
  void begin(const char *deviceName, const char *serviceUuid,
             const char *sensorCharUuid);
  void notify(const uint8_t *data, size_t len);

private:
  NimBLECharacteristic *sensorChar_ = nullptr;
};
