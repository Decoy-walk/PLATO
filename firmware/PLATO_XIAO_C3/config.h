#pragma once

// ---- BLE ----
#define BLE_DEVICE_NAME      "PLATO-BLOCK-C3"
#define BLE_SERVICE_UUID     "6f2a0001-8b1e-4a3e-9d0a-0000000000a1"
#define BLE_SENSOR_CHAR_UUID "6f2a0002-8b1e-4a3e-9d0a-0000000000a1"

// ---- Flex sensor mux ----
// 20 flex sensors (one per hinge) read through two CD74HC4067 16-channel
// analog muxes sharing one ADC pin: XIAO ESP32C3 only exposes 3-4 ADC1
// pins, nowhere near 20, so channel count is time-multiplexed down to a
// single analog input instead of one ADC channel per sensor.
//   - Address lines S0-S3 are wired in parallel to both mux chips.
//   - Mux A's active-low EN selects hinges 0-15, mux B's EN selects
//     hinges 16-19 (only 4 of its 16 channels are used) - only one EN is
//     ever driven low at a time, the other is left high (output disabled).
//   - Both chips' common SIG pin ties to the single MUX_ADC_PIN.
#define MUX_ADC_PIN  D0 // GPIO2, ADC1_CH2
#define MUX_S0_PIN   D1 // GPIO3
#define MUX_S1_PIN   D2 // GPIO4
#define MUX_S2_PIN   D3 // GPIO5
#define MUX_S3_PIN   D4 // GPIO6
#define MUX_EN_A_PIN D5 // GPIO7  - active-low enable, mux A = hinges 0-15
#define MUX_EN_B_PIN D6 // GPIO21 - active-low enable, mux B = hinges 16-19

#define HAPTIC_PIN     D7  // GPIO20 - PWM to the haptic/ERM motor driver transistor
#define STATUS_LED_PIN D10 // GPIO10 - lit while any hinge reads folded

#define MUX_SETTLE_US 50 // let the mux output settle before analogRead

// ---- Fold detection ----
// Normalized (0-1, from per-hinge calibration) threshold above which a
// hinge counts as "folded". Send 'c' over serial to (re)calibrate all 20
// hinges before each session - per-hinge variance in the FSR/flex divider
// output is exactly the internal-validity concern raised for the write-up.
#define FOLD_THRESHOLD 0.5f

// ---- Timing ----
#define SCAN_INTERVAL_MS        20    // ~50 Hz full 20-channel scan
#define BLE_NOTIFY_INTERVAL_MS  50    // ~20 Hz BLE notify of the fold bitmask
#define CALIBRATION_DURATION_MS 15000 // hold-the-serial-command calibration window

// ---- Haptic feedback ("Block + haptic feedback" vs "Block motor-only") ----
// Toggle at runtime with the 'h' serial command so the same firmware
// serves both conditions without reflashing between participants.
#define HAPTIC_ENABLED_DEFAULT true
#define HAPTIC_INTENSITY 180 // 0-255 PWM duty
#define HAPTIC_PULSE_MS  80  // vibration burst length per fold-state change
