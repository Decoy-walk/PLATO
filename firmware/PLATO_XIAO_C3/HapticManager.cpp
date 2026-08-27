#include "HapticManager.h"

void HapticManager::begin(uint8_t pin) {
  pin_ = pin;
  pinMode(pin_, OUTPUT);
  analogWrite(pin_, 0);
}

void HapticManager::pulse(uint8_t intensity, uint32_t durationMs) {
  analogWrite(pin_, intensity);
  offAtMs_ = millis() + durationMs;
  active_ = true;
}

void HapticManager::update() {
  if (active_ && millis() >= offAtMs_) {
    analogWrite(pin_, 0);
    active_ = false;
  }
}
