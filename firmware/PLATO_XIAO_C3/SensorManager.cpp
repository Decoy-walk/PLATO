#include "SensorManager.h"

#include <Wire.h>

bool SensorManager::begin(uint8_t sdaPin, uint8_t sclPin, uint8_t i2cAddr) {
  Wire.begin(sdaPin, sclPin);
  reading_.valid = bme_.begin(i2cAddr, &Wire);
  if (!reading_.valid) {
    Serial.println("[SensorManager] BME280 not found, check wiring/address");
  }
  return reading_.valid;
}

void SensorManager::update() {
  if (!reading_.valid) return;
  reading_.temperatureC = bme_.readTemperature();
  reading_.humidityPct = bme_.readHumidity();
  reading_.pressureHpa = bme_.readPressure() / 100.0F;
}

String SensorManager::toJson() const {
  String json = "{";
  json += "\"valid\":" + String(reading_.valid ? "true" : "false") + ",";
  json += "\"temperature_c\":" + String(reading_.temperatureC, 2) + ",";
  json += "\"humidity_pct\":" + String(reading_.humidityPct, 2) + ",";
  json += "\"pressure_hpa\":" + String(reading_.pressureHpa, 2);
  json += "}";
  return json;
}
