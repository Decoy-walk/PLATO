// Plato_004_fsr_led_buzzer_vibration_lift.ino
//
// Extends Plato_002_fsr_led_buzzer_vibration with a fourth output: a
// servo-driven slider-crank mechanism that physically lifts/lowers the
// resonance plate while pressed, instead of only buzzing/vibrating it -
// a stronger, more deliberate "the plate is pulsing" haptic gimmick on
// top of the existing sound/vibration layers.
//
// FSR->LED brightness, the buzzer's chirp pattern, and the vibration
// motor's force-proportional intensity are all unchanged from Plato_002 -
// see that folder's README for the reasoning behind each. This file only
// adds the servo/lift layer.
//
// Board: Tools > Board > Seeed SAMD Boards > Seeeduino XIAO
//
// Mechanism: slider-crank (same principle as a piston engine, but
// swinging back and forth across a limited arc instead of spinning a
// full 360 degrees - that's fine, a slider-crank doesn't need continuous
// rotation to produce reciprocating linear motion, it only needs the
// crank pin to sweep through some arc):
//   - Servo horn = the crank. A short pin/screw mounted off-center on the
//     horn (crank radius, try ~10mm to start) traces an arc as the servo
//     sweeps between SERVO_MIN_ANGLE and SERVO_MAX_ANGLE.
//   - A rigid connecting rod (try ~40mm, roughly 3-4x the crank radius so
//     it doesn't bind near the limits) links that pin to the slider.
//   - The slider itself must be constrained to vertical-only motion by
//     its own guide (two vertical printed rails/rods through sleeves on
//     the slider, or a drawer-slide) - the connecting rod's end on the
//     slider only needs a simple pin joint, the guide is what keeps the
//     slider from swinging sideways with the crank.
//   - The resonance plate sits on/attaches to the slider.
//   Start with SERVO_MIN_ANGLE/SERVO_MAX_ANGLE narrower than the servo's
//   full ~160 degree range (e.g. 20-140) so the linkage never hits its
//   own mechanical end stops; widen gradually once the real geometry (rod
//   length, pin placement, guide friction) is tuned on the bench - this
//   is a physical-build parameter, not something to get right on paper.
//
// Wiring (adds to the Plato_002_fsr_led_buzzer_vibration breadboard - see
// that folder's README for the FSR/LED/buzzer/vibration rows):
//   Servo (ROB-14760, 3-wire: signal/power/ground):
//     D1      -> signal (yellow/white wire)
//     XIAO "5V" pin -> power (red wire)   (this servo is rated 4.8-6V, the
//                                           same reason the buzzer needed
//                                           the 5V pin instead of 3V3 -
//                                           see Plato_002's README. Needs
//                                           the board USB-powered.)
//     GND     -> ground (brown wire)
//   The signal wire runs fine at the XIAO's 3.3V logic level even though
//   the servo itself is powered at 5V - practically all hobby servos'
//   pulse-decoder ICs recognize anything above ~2-2.5V as a HIGH pulse,
//   so no level-shifting is needed for the signal line, only the power
//   wire needs the 5V pin.
//   D1 was picked (not D6/D7) because the Seeed SAMD core's Servo library
//   port has a known timer conflict with analogWrite() specifically on
//   pins 6 and 7 - unrelated to any pin already used here, but worth
//   avoiding if you relocate other outputs later.

#include <Servo.h>

const int fsrPin = A0;
const int ledPin = 9;
const int buzzerPin = 8;
const int vibrationPin = 2;
const int servoPin = 1;

const float R_FIXED = 1000.0; // 1k ohm
const float VCC = 3.3;        // XIAO SAMD21 runs its ADC/IO at 3.3V

// FSR402 conductance range observed on the bench; re-measure per physical
// build (see Plato_001_fsr_led/README.md's calibration section).
float condMin = 0.000007;
float condMax = 0.001;

const int PRESS_ON_BRIGHTNESS = 40;  // brightness (0-255) above which the buzzer/vibration/lift turn on
const int PRESS_OFF_BRIGHTNESS = 20; // below which they turn back off
// (the gap between ON/OFF is hysteresis, so sensor noise right at the
// threshold can't make them chatter on and off rapidly)

// Small coin ERM motors typically need roughly 60-70% of their rated
// voltage just to start spinning at all, so intensity is scaled within
// this upper band instead of the full 0-255 range (see Plato_002's README).
const int MIN_EFFECTIVE_VIBRATION = 170; // PWM floor once pressed - below this it may not spin
const int MAX_VIBRATION = 255;           // PWM at full press force

const unsigned long SENSE_INTERVAL_MS = 50; // FSR read / LED update rate
// See Plato_002's README "Making the buzzer sound less harsh" for why
// these two values (not a continuous tone, not too short to be quiet).
const unsigned long BUZZ_ON_MS = 90;  // each chirp's ON duration
const unsigned long BUZZ_OFF_MS = 35; // gap between chirps while pressed

// Lift (servo slider-crank) tuning - see the mechanism note above.
const int SERVO_MIN_ANGLE = 20;   // plate's lowered position
const int SERVO_MAX_ANGLE = 140;  // plate's raised position
const int SERVO_STEP_DEGREES = 2; // how far the servo moves per step
// Sweep speed scales with press force, same idea as the vibration motor:
// a light press lifts slowly, a hard press lifts/lowers quickly. These
// are step *intervals* (smaller = faster), not speeds, so MIN < MAX here.
const unsigned long SERVO_STEP_INTERVAL_MIN_MS = 8;  // fastest (hardest press)
const unsigned long SERVO_STEP_INTERVAL_MAX_MS = 30; // slowest (just-pressed)

Servo liftServo;

bool pressed = false;
bool buzzChirpOn = false;
int brightness = 0;
int servoAngle;
int servoDirection = 1; // +1 raising, -1 lowering
unsigned long lastSenseMs = 0;
unsigned long lastBuzzToggleMs = 0;
unsigned long lastServoStepMs = 0;

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(fsrPin, INPUT);
  pinMode(buzzerPin, INPUT); // high-impedance = buzzer off (see Plato_002's README)
  pinMode(vibrationPin, OUTPUT);

  servoAngle = SERVO_MIN_ANGLE;
  liftServo.attach(servoPin);
  liftServo.write(servoAngle); // start lowered
}

void loop() {
  unsigned long now = millis();

  if (now - lastSenseMs >= SENSE_INTERVAL_MS) {
    lastSenseMs = now;

    int raw = analogRead(fsrPin);
    float vOut = raw * (VCC / 1023.0);

    float conductance;
    if (vOut <= 0.01) {
      conductance = 0;
    } else {
      float rFsr = R_FIXED * (VCC - vOut) / vOut;
      conductance = 1.0 / rFsr;
    }

    brightness = (int)((conductance - condMin) / (condMax - condMin) * 255);
    brightness = constrain(brightness, 0, 255);
    analogWrite(ledPin, brightness);

    if (!pressed && brightness > PRESS_ON_BRIGHTNESS) {
      pressed = true;
    } else if (pressed && brightness < PRESS_OFF_BRIGHTNESS) {
      pressed = false;
    }

    if (pressed) {
      int vibrationIntensity = map(brightness, PRESS_ON_BRIGHTNESS, 255,
                                    MIN_EFFECTIVE_VIBRATION, MAX_VIBRATION);
      vibrationIntensity = constrain(vibrationIntensity, MIN_EFFECTIVE_VIBRATION, MAX_VIBRATION);
      analogWrite(vibrationPin, vibrationIntensity);
    } else {
      analogWrite(vibrationPin, 0);
    }

    Serial.print("raw: ");
    Serial.print(raw);
    Serial.print("\tconductance: ");
    Serial.print(conductance, 6);
    Serial.print("\tbrightness: ");
    Serial.print(brightness);
    Serial.print("\tpressed: ");
    Serial.println(pressed);
  }

  if (pressed) {
    if (buzzChirpOn && now - lastBuzzToggleMs >= BUZZ_ON_MS) {
      buzzChirpOn = false;
      lastBuzzToggleMs = now;
      pinMode(buzzerPin, INPUT); // silent gap between chirps
    } else if (!buzzChirpOn && now - lastBuzzToggleMs >= BUZZ_OFF_MS) {
      buzzChirpOn = true;
      lastBuzzToggleMs = now;
      pinMode(buzzerPin, OUTPUT);
      digitalWrite(buzzerPin, LOW); // sinks current, completing the 5V-fed buzzer's circuit
    }
  } else if (buzzChirpOn) {
    buzzChirpOn = false;
    pinMode(buzzerPin, INPUT); // high-impedance: no current path at all, so it's reliably silent
  }

  if (pressed) {
    unsigned long stepInterval = map(brightness, PRESS_ON_BRIGHTNESS, 255,
                                      SERVO_STEP_INTERVAL_MAX_MS, SERVO_STEP_INTERVAL_MIN_MS);
    stepInterval = constrain(stepInterval, SERVO_STEP_INTERVAL_MIN_MS, SERVO_STEP_INTERVAL_MAX_MS);
    if (now - lastServoStepMs >= stepInterval) {
      lastServoStepMs = now;
      servoAngle += servoDirection * SERVO_STEP_DEGREES;
      if (servoAngle >= SERVO_MAX_ANGLE) {
        servoAngle = SERVO_MAX_ANGLE;
        servoDirection = -1;
      } else if (servoAngle <= SERVO_MIN_ANGLE) {
        servoAngle = SERVO_MIN_ANGLE;
        servoDirection = 1;
      }
      liftServo.write(servoAngle);
    }
  } else if (servoAngle != SERVO_MIN_ANGLE) {
    servoAngle = SERVO_MIN_ANGLE;
    servoDirection = 1;
    liftServo.write(servoAngle); // lower the plate as soon as the press ends
  }
}
