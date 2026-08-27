#include "BleManager.h"

void BleManager::begin(const char *deviceName, const char *serviceUuid,
                        const char *sensorCharUuid) {
  NimBLEDevice::init(deviceName);

  NimBLEServer *server = NimBLEDevice::createServer();
  NimBLEService *service = server->createService(serviceUuid);

  sensorChar_ = service->createCharacteristic(
      sensorCharUuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  service->start();

  NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(serviceUuid);
  advertising->start();
}

void BleManager::notifySensor(const String &json) {
  if (sensorChar_ == nullptr) return;
  // Wrap explicitly in std::string: NimBLECharacteristic::setValue() also has
  // a generic template overload that a bare const char* can bind to,
  // copying the pointer's bytes instead of the string it points to.
  sensorChar_->setValue(std::string(json.c_str(), json.length()));
  sensorChar_->notify();
}
