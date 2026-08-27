#include <Arduino.h>

#include "BioSensorManager.h"
#include "BleManager.h"
#include "config.h"

BioSensorManager sensors;
BleManager ble;

uint32_t lastNotifyMs = 0;

void setup() {
  Serial.begin(115200);
  uint32_t bootStart = millis();
  while (!Serial && millis() - bootStart < 2000) {
    delay(10);
  }

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  sensors.begin(FSR_PIN, PPG_PIN, PPG_LED_PIN, AMBIENT_PIN);
  ble.begin(BLE_DEVICE_NAME, BLE_SERVICE_UUID, BLE_SENSOR_CHAR_UUID);

  Serial.println("[PLATO] Sensing node ready");
}

void loop() {
  sensors.sample();
  digitalWrite(STATUS_LED_PIN, sensors.gripDetected() ? HIGH : LOW);

  uint32_t now = millis();
  if (now - lastNotifyMs >= BLE_NOTIFY_INTERVAL_MS) {
    lastNotifyMs = now;
    String json = sensors.toJson();
    ble.notifySensor(json);
    Serial.println(json);
  }
}
