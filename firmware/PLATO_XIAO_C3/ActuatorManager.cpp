#include "ActuatorManager.h"

void ActuatorManager::begin(uint8_t ledPin, uint8_t servoPin,
                             uint8_t motorIn1Pin, uint8_t motorIn2Pin) {
  ledPin_ = ledPin;
  motorIn1Pin_ = motorIn1Pin;
  motorIn2Pin_ = motorIn2Pin;

  pinMode(ledPin_, OUTPUT);
  digitalWrite(ledPin_, LOW);

  pinMode(motorIn1Pin_, OUTPUT);
  pinMode(motorIn2Pin_, OUTPUT);
  analogWrite(motorIn1Pin_, 0);
  analogWrite(motorIn2Pin_, 0);

  servo_.setPeriodHertz(50);
  servo_.attach(servoPin, 500, 2400);
  servo_.write(servoAngle_);
}

void ActuatorManager::setLed(bool on) {
  ledOn_ = on;
  digitalWrite(ledPin_, on ? HIGH : LOW);
}

void ActuatorManager::setServoAngle(int angleDeg) {
  servoAngle_ = constrain(angleDeg, 0, 180);
  servo_.write(servoAngle_);
}

void ActuatorManager::setMotorSpeed(int speed) {
  motorSpeed_ = constrain(speed, -255, 255);
  if (motorSpeed_ >= 0) {
    analogWrite(motorIn1Pin_, motorSpeed_);
    analogWrite(motorIn2Pin_, 0);
  } else {
    analogWrite(motorIn1Pin_, 0);
    analogWrite(motorIn2Pin_, -motorSpeed_);
  }
}
