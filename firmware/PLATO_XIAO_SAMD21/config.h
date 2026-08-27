#pragma once

// ---- Serial transport (wired, interim pending the wireless decision) ----
// SAMD21 has no on-board radio, so this build streams over the native USB
// serial port instead of BLE. Each frame is [2 sync bytes, 3 bytes fold
// bitmask (LSB first), 1 flags byte] so the PC-side reader can stay
// byte-aligned on a continuous stream (unlike BLE notify, which is
// naturally framed per-packet); two marker bytes (vs. one) make an
// accidental match inside the bitmask itself far less likely to cause a
// false resync. If/when a BLE UART module is added, only the transport
// needs to change (see SerialTransport.*) - FlexMuxManager and
// HapticManager are unaffected.
#define SERIAL_BAUD 115200
#define SERIAL_SYNC_BYTE_1 0xAA
#define SERIAL_SYNC_BYTE_2 0x55

// ---- Flex sensor mux ----
// 20 flex sensors (one per hinge) read through two CD74HC4067 16-channel
// analog muxes sharing one ADC pin. Unlike ESP32C3, SAMD21 has no
// radio-vs-ADC pin conflict to work around - any pin works - but the wiring
// is kept identical to the ESP32C3 build for continuity with the physical
// harness.
#define MUX_ADC_PIN  0  // A0/D0
#define MUX_S0_PIN   1  // D1
#define MUX_S1_PIN   2  // D2
#define MUX_S2_PIN   3  // D3
#define MUX_S3_PIN   4  // D4
#define MUX_EN_A_PIN 5  // D5  - active-low enable, mux A = hinges 0-15
#define MUX_EN_B_PIN 6  // D6  - active-low enable, mux B = hinges 16-19

#define HAPTIC_PIN     7  // D7  - PWM to the haptic/ERM motor driver transistor
#define STATUS_LED_PIN 10 // D10 - lit while any hinge reads folded

#define MUX_SETTLE_US 50 // let the mux output settle before analogRead

// ---- Fold detection ----
// Normalized (0-1, from per-hinge calibration) threshold above which a
// hinge counts as "folded". Send 'c' over serial to (re)calibrate all 20
// hinges before each session - calibration is RAM-only (see
// FlexMuxManager) and needs re-running after every power cycle, which
// matches the documented per-session calibration protocol anyway.
#define FOLD_THRESHOLD 0.5f

// ---- Timing ----
#define SCAN_INTERVAL_MS        20    // ~50 Hz full 20-channel scan
#define NOTIFY_INTERVAL_MS      50    // ~20 Hz serial notify of the fold bitmask
#define CALIBRATION_DURATION_MS 15000 // hold-the-serial-command calibration window

// ---- Haptic feedback ("Block + haptic feedback" vs "Block motor-only") ----
// Toggle at runtime with the 'h' serial command so the same flashed
// firmware serves both conditions without reflashing between participants.
#define HAPTIC_ENABLED_DEFAULT true
#define HAPTIC_INTENSITY 180 // 0-255 PWM duty
#define HAPTIC_PULSE_MS  80  // vibration burst length per fold-state change
