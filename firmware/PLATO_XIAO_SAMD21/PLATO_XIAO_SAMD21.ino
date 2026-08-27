#include <Arduino.h>

#include "FlexMuxManager.h"
#include "HapticManager.h"
#include "SerialTransport.h"
#include "config.h"

FlexMuxManager flex;
HapticManager haptic;
SerialTransport transport;

bool hapticEnabled = HAPTIC_ENABLED_DEFAULT;
uint32_t lastScanMs = 0;
uint32_t lastNotifyMs = 0;
uint32_t currentBitmask = 0;

// 'c' = (re)calibrate all 20 hinges, 'h' = toggle the haptic-feedback
// condition at runtime so the same flashed firmware serves both the
// motor-only and haptic-feedback study arms.
void handleSerialCommands() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == 'c' || c == 'C') {
    flex.calibrate(CALIBRATION_DURATION_MS);
  } else if (c == 'h' || c == 'H') {
    hapticEnabled = !hapticEnabled;
    Serial.println(hapticEnabled ? "[PLATO] Haptic feedback ENABLED"
                                  : "[PLATO] Haptic feedback disabled");
  }
}

void setup() {
  transport.begin(SERIAL_BAUD);
  uint32_t bootStart = millis();
  while (!Serial && millis() - bootStart < 2000) {
    delay(10);
  }

  analogReadResolution(12); // match the ESP32C3 build's ADC precision

  flex.begin(MUX_ADC_PIN, MUX_S0_PIN, MUX_S1_PIN, MUX_S2_PIN, MUX_S3_PIN,
             MUX_EN_A_PIN, MUX_EN_B_PIN);
  haptic.begin(HAPTIC_PIN);

  pinMode(STATUS_LED_PIN, OUTPUT);

  Serial.println("[PLATO] Folding-block node ready (wired serial build).");
  Serial.println("  'c' = (re)calibrate all 20 hinges, 'h' = toggle haptic feedback");
  Serial.println("  Calibration is RAM-only - run 'c' again after every power cycle.");
}

void loop() {
  handleSerialCommands();

  uint32_t now = millis();

  if (now - lastScanMs >= SCAN_INTERVAL_MS) {
    lastScanMs = now;

    uint32_t previousBitmask = currentBitmask;
    flex.scan();
    currentBitmask = flex.foldBitmask();
    digitalWrite(STATUS_LED_PIN, currentBitmask != 0 ? HIGH : LOW);

    if (hapticEnabled && currentBitmask != previousBitmask) {
      haptic.pulse(HAPTIC_INTENSITY, HAPTIC_PULSE_MS);
    }
  }

  haptic.update();

  if (now - lastNotifyMs >= NOTIFY_INTERVAL_MS) {
    lastNotifyMs = now;
    // 3 bytes: 20-bit fold bitmask (LSB first). 4th byte: bit0 = haptic
    // pulse currently firing, bit1 = haptic-feedback condition enabled.
    uint8_t payload[4] = {
        (uint8_t)(currentBitmask & 0xFF),
        (uint8_t)((currentBitmask >> 8) & 0xFF),
        (uint8_t)((currentBitmask >> 16) & 0xFF),
        (uint8_t)((haptic.isActive() ? 0x01 : 0x00) |
                  (hapticEnabled ? 0x02 : 0x00)),
    };
    transport.send(payload, sizeof(payload));
  }
}
