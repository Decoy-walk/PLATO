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

void BleManager::notify(const uint8_t *data, size_t len) {
  if (sensorChar_ == nullptr) return;
  sensorChar_->setValue(data, len);
  sensorChar_->notify();
}
