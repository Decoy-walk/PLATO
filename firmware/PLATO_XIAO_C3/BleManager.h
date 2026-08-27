#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>

// BLE GATT service that streams derived interaction features (grip force,
// heart rate) as JSON notifications for a PC-side logging bridge.
class BleManager {
public:
  void begin(const char *deviceName, const char *serviceUuid,
             const char *sensorCharUuid);
  void notifySensor(const String &json);

private:
  NimBLECharacteristic *sensorChar_ = nullptr;
};
