#include "BleManager.h"

void BleManager::ControlCallbacks::onWrite(NimBLECharacteristic *characteristic) {
  if (cb_) {
    cb_(String(characteristic->getValue().c_str()));
  }
}

void BleManager::begin(const char *deviceName, const char *serviceUuid,
                        const char *sensorCharUuid,
                        const char *controlCharUuid,
                        ControlCallback onControl) {
  NimBLEDevice::init(deviceName);

  NimBLEServer *server = NimBLEDevice::createServer();
  NimBLEService *service = server->createService(serviceUuid);

  sensorChar_ = service->createCharacteristic(
      sensorCharUuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  controlChar_ =
      service->createCharacteristic(controlCharUuid, NIMBLE_PROPERTY::WRITE);
  controlCallbacks_ = new ControlCallbacks(onControl);
  controlChar_->setCallbacks(controlCallbacks_);

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
