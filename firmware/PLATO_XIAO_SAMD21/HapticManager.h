#pragma once

#include <Arduino.h>

// Non-blocking control for a single ERM/LRA haptic actuator driven through
// a transistor from a PWM-capable pin - the feedback channel for the
// "Block + haptic feedback" condition.
class HapticManager {
public:
  void begin(uint8_t pin);
  void pulse(uint8_t intensity, uint32_t durationMs);
  void update(); // call every loop(); turns the actuator off after durationMs

  bool isActive() const { return active_; }

private:
  uint8_t pin_ = 0;
  uint32_t offAtMs_ = 0;
  bool active_ = false;
};
