#include <Arduino.h>
#include <ArduinoJson.h>

#include "ActuatorManager.h"
#include "BleManager.h"
#include "NetManager.h"
#include "SensorManager.h"
#include "config.h"

SensorManager sensors;
ActuatorManager actuators;
NetManager net;
BleManager ble;

uint32_t lastSensorReadMs = 0;

String buildStatusJson() {
  String json = "{";
  json += "\"sensor\":" + sensors.toJson() + ",";
  json += "\"led\":" + String(actuators.ledState() ? "true" : "false") + ",";
  json += "\"servo\":" + String(actuators.servoAngle()) + ",";
  json += "\"motor\":" + String(actuators.motorSpeed());
  json += "}";
  return json;
}

// Shared control protocol for both HTTP POST /api/control and the BLE
// control characteristic: {"led":true,"servo":90,"motor":150} (any subset).
void applyControlJson(const String &json) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("[Control] JSON parse failed: %s\n", err.c_str());
    return;
  }

  if (!doc["led"].isNull()) {
    actuators.setLed(doc["led"].as<bool>());
  }
  if (!doc["servo"].isNull()) {
    actuators.setServoAngle(doc["servo"].as<int>());
  }
  if (!doc["motor"].isNull()) {
    actuators.setMotorSpeed(doc["motor"].as<int>());
  }
}

void setup() {
  Serial.begin(115200);
  uint32_t bootStart = millis();
  while (!Serial && millis() - bootStart < 2000) {
    delay(10);
  }

  actuators.begin(STATUS_LED_PIN, SERVO_PIN, MOTOR_IN1_PIN, MOTOR_IN2_PIN);
  sensors.begin(I2C_SDA_PIN, I2C_SCL_PIN, BME280_ADDR);

  net.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CONNECT_TIMEOUT_MS, buildStatusJson,
            applyControlJson);

  ble.begin(BLE_DEVICE_NAME, BLE_SERVICE_UUID, BLE_SENSOR_CHAR_UUID,
            BLE_CONTROL_CHAR_UUID, applyControlJson);

  Serial.println("[PLATO] Ready");
}

void loop() {
  net.loop();

  uint32_t now = millis();
  if (now - lastSensorReadMs >= SENSOR_READ_INTERVAL_MS) {
    lastSensorReadMs = now;
    sensors.update();
    ble.notifySensor(sensors.toJson());
  }
}
