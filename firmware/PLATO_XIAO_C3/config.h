#pragma once

// ---- BLE ----
#define BLE_DEVICE_NAME      "PLATO-SENSE-C3"
#define BLE_SERVICE_UUID     "6f2a0001-8b1e-4a3e-9d0a-0000000000a1"
#define BLE_SENSOR_CHAR_UUID "6f2a0002-8b1e-4a3e-9d0a-0000000000a1"

// ---- Pins (Seeed XIAO ESP32C3 silkscreen labels) ----
// FSR/PPG/ambient all sit on ADC1-capable pins (GPIO2-4) so they keep
// reading correctly while the BLE radio is active (ADC2 is unusable
// alongside RF on this chip).
#define FSR_PIN        D0 // GPIO2, ADC1_CH2 - FSR402 (short tail) voltage divider
#define PPG_PIN        D1 // GPIO3, ADC1_CH3 - NJL5513R reflectance PPG output
#define AMBIENT_PIN    D2 // GPIO4, ADC1_CH4 - BPW34 ambient-light reference
#define PPG_LED_PIN    D3 // GPIO5 - drives the NJL5513R reflectance LED
#define STATUS_LED_PIN D4 // GPIO6 - grip feedback LED

// ---- Sampling ----
#define SAMPLE_INTERVAL_MS     4   // ~250 Hz raw sampling for the PPG channel
#define BLE_NOTIFY_INTERVAL_MS 100 // ~10 Hz derived-feature stream over BLE

// ---- FSR402 grip/press force ----
// Voltage divider: 3V3 -- FSR402 -- ADC_PIN -- 10k -- GND
#define FSR_GRIP_THRESHOLD 200 // raw ADC counts (0-4095); tune to your divider/grip

// ---- PPG beat detection (NJL5513R, ambient-cancelled with BPW34) ----
#define PPG_BASELINE_ALPHA      0.01f // slow envelope tracking DC/ambient drift
#define PPG_ENVELOPE_ALPHA      0.3f  // fast envelope of the AC pulse component
#define PPG_AMBIENT_CANCEL_GAIN 1.0f  // how much of the BPW34 ambient swing to subtract
#define PPG_MIN_BEAT_MS         300   // refractory period, caps detection at 200 bpm
#define PPG_BEAT_THRESHOLD      6.0f  // AC signal units above baseline to count a beat
