#pragma once

#include <Adafruit_BME280.h>
#include <Arduino.h>

struct SensorReading {
  float temperatureC = NAN;
  float humidityPct = NAN;
  float pressureHpa = NAN;
  bool valid = false;
};

// Wraps a BME280 temperature/humidity/pressure sensor on I2C.
class SensorManager {
public:
  bool begin(uint8_t sdaPin, uint8_t sclPin, uint8_t i2cAddr);
  void update();

  const SensorReading &reading() const { return reading_; }
  String toJson() const;

private:
  Adafruit_BME280 bme_;
  SensorReading reading_;
};
