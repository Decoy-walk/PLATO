// PLATO -- exhibition build (Seeeduino XIAO, SAMD21)
//
// Standalone interactive piece for the folding ball: a flex sensor tracks
// how open/closed it is, and an LED brightens as it opens. No wireless, no
// PC bridge, no per-hinge sensing -- this is a separate build from the
// research firmware in firmware/PLATO_XIAO_C3/, for the exhibition demo
// only. Zero external library dependencies (Arduino SAMD core only).
//
// Wiring:
//   Flex sensor: 3V3 -- flex sensor -- A0 -- 10k resistor -- GND
//   LED:         D1 -- 330 ohm resistor -- LED anode; LED cathode -- GND
//
// Board: Tools > Board > Seeed SAMD Boards > Seeeduino XIAO

#include <Arduino.h>

const uint8_t FLEX_PIN = A0;
const uint8_t LED_PIN = 1; // D1, PWM-capable

const float SMOOTH_ALPHA = 0.15f;          // low-pass filter on the raw ADC reading
const uint16_t CAL_MIN_RANGE = 20;         // ignore ranges this small (pre-calibration noise)
const uint32_t IDLE_TIMEOUT_MS = 4000;     // no significant movement -> idle breathing
const float IDLE_MOVEMENT_THRESHOLD = 0.03f; // openness delta that counts as "being touched"
const uint8_t IDLE_PEAK_BRIGHTNESS = 60;   // dim ambient glow while idle

float smoothed = 0;
float calMin = 0;
float calMax = 4095;
float lastOpenness = 0;
uint32_t lastMovementMs = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  analogReadResolution(12);

  smoothed = analogRead(FLEX_PIN);
  calMin = smoothed;
  calMax = smoothed + 1; // avoid a zero-width range before the first real movement
  lastMovementMs = millis();
}

void loop() {
  int raw = analogRead(FLEX_PIN);
  smoothed += SMOOTH_ALPHA * (raw - smoothed);

  // Auto-range: widen to fit whatever extremes visitors actually produce.
  // Only ever widens, never narrows, so this can't destabilize mid-exhibit
  // and needs no manual calibration step each morning.
  if (smoothed < calMin) calMin = smoothed;
  if (smoothed > calMax) calMax = smoothed;

  float range = calMax - calMin;
  float openness = range > CAL_MIN_RANGE ? (smoothed - calMin) / range : 0.0f;
  openness = constrain(openness, 0.0f, 1.0f);

  uint32_t now = millis();
  if (fabsf(openness - lastOpenness) > IDLE_MOVEMENT_THRESHOLD) {
    lastMovementMs = now;
  }
  lastOpenness = openness;

  uint8_t brightness;
  if (now - lastMovementMs > IDLE_TIMEOUT_MS) {
    // Idle: gentle breathing so the piece still feels alive when nobody's
    // touching it, instead of just going dark.
    float phase = (now % 3000) / 3000.0f;
    float breathe = (sinf(phase * 2.0f * PI) + 1.0f) * 0.5f; // 0..1
    brightness = (uint8_t)(breathe * IDLE_PEAK_BRIGHTNESS);
  } else {
    brightness = (uint8_t)(openness * 255);
  }

  analogWrite(LED_PIN, brightness);
  delay(10);
}
