#pragma once

// ---- Wi-Fi ----
#define WIFI_SSID               "YOUR_WIFI_SSID"
#define WIFI_PASSWORD           "YOUR_WIFI_PASSWORD"
#define WIFI_CONNECT_TIMEOUT_MS 15000

// ---- BLE ----
#define BLE_DEVICE_NAME       "PLATO-XIAO-C3"
#define BLE_SERVICE_UUID      "6f2a0001-8b1e-4a3e-9d0a-0000000000a1"
#define BLE_SENSOR_CHAR_UUID  "6f2a0002-8b1e-4a3e-9d0a-0000000000a1"
#define BLE_CONTROL_CHAR_UUID "6f2a0003-8b1e-4a3e-9d0a-0000000000a1"

// ---- Pins (Seeed XIAO ESP32C3 silkscreen labels) ----
// I2C bus for the BME280 sensor (default Wire pins on this board)
#define I2C_SDA_PIN D4 // GPIO6
#define I2C_SCL_PIN D5 // GPIO7
#define BME280_ADDR 0x76

// External status LED (+ series resistor to GND). GPIO2 is a strapping pin;
// keep the LED load light so it doesn't interfere with boot.
#define STATUS_LED_PIN D0 // GPIO2

// Servo signal line (needs its own 5V supply for anything but a micro servo).
#define SERVO_PIN D2 // GPIO4

// DC motor driver (e.g. DRV8833/TB6612): two PWM-capable direction inputs.
// Forward = PWM on IN1 with IN2 low, reverse = PWM on IN2 with IN1 low.
#define MOTOR_IN1_PIN D1 // GPIO3
#define MOTOR_IN2_PIN D3 // GPIO5

// ---- Timing ----
#define SENSOR_READ_INTERVAL_MS 2000
