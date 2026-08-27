#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>

// Drives a status LED, a hobby servo, and a brushed DC motor through a
// two-pin H-bridge driver (DRV8833/TB6612-style).
class ActuatorManager {
public:
  void begin(uint8_t ledPin, uint8_t servoPin, uint8_t motorIn1Pin,
             uint8_t motorIn2Pin);

  void setLed(bool on);
  void setServoAngle(int angleDeg);
  void setMotorSpeed(int speed); // -255..255, negative = reverse

  bool ledState() const { return ledOn_; }
  int servoAngle() const { return servoAngle_; }
  int motorSpeed() const { return motorSpeed_; }

private:
  Servo servo_;
  uint8_t ledPin_ = 0;
  uint8_t motorIn1Pin_ = 0;
  uint8_t motorIn2Pin_ = 0;
  bool ledOn_ = false;
  int servoAngle_ = 90;
  int motorSpeed_ = 0;
};
