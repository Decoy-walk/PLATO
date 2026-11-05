// Plato_005_tact_lift.ino
//
// A simplified, standalone build on a fresh breadboard: a tact switch
// (momentary pushbutton) replaces the FSR402 as the trigger, and the
// servo slider-crank lift mechanism from
// Plato_004_fsr_led_buzzer_vibration_lift is the only output - no LED,
// buzzer, or vibration motor here. Pressing the switch toggles the lift
// on/off: press once, it starts cycling up and down continuously on its
// own; press again, it stops and returns to its lowered rest position.
//
// Mechanism: same slider-crank linkage as Plato_004 (see that folder's
// README and hardware/cad/plato_004_lift_parts.dxf for the physical
// build - nothing about the mechanism itself changes here, only the
// control board/firmware).
//
// Board: Tools > Board > Seeed SAMD Boards > Seeeduino XIAO
//
// Wiring (new breadboard, just two components):
//   Tact switch:
//     D2  -> one leg of the switch
//     GND -> the switch's other leg
//   Read with INPUT_PULLUP (D2 idles HIGH internally; pressing the
//   switch connects it straight to GND, pulling it LOW) - no external
//   resistor needed for this.
//   Servo (ROB-14760, 3-wire):
//     D1            -> signal (yellow/white)
//     XIAO "5V" pin -> power (red)    (rated 4.8-6V, needs the raw USB
//                                       VBUS "5V" pin, not 3V3 - same
//                                       reasoning as Plato_002's buzzer
//                                       and Plato_004's servo. Needs the
//                                       board USB-powered.)
//     GND           -> ground (brown)
//
// Debouncing: a tact switch's contacts physically "bounce" for a few ms
// on each press/release, which a microcontroller reads fast enough to
// see as several rapid transitions instead of one clean edge - without
// debouncing, a single physical press could toggle the lift on and off
// several times in a row. DEBOUNCE_MS requires the reading to stay
// stable for that long before a transition counts as real.

#include <Servo.h>

const int switchPin = 2;
const int servoPin = 1;

const int SERVO_MIN_ANGLE = 20;   // plate's lowered/rest position
const int SERVO_MAX_ANGLE = 140;  // plate's raised position
const int SERVO_STEP_DEGREES = 2; // how far the servo moves per internal step
// Fixed sweep speed - there's no analog force signal here to grade it by
// (unlike Plato_002/004's vibration motor and lift, which scale with FSR
// press force). Pick a value roughly in the middle of Plato_004's old
// 8-30ms force-scaled range.
const unsigned long SERVO_STEP_INTERVAL_MS = 15;

const unsigned long DEBOUNCE_MS = 30; // typical tact switch debounce window

Servo liftServo;

bool liftRunning = false;
int servoAngle;
int servoDirection = 1; // +1 raising, -1 lowering
unsigned long lastServoStepMs = 0;

bool lastRawState = HIGH;     // INPUT_PULLUP idles HIGH (not pressed)
bool debouncedState = HIGH;
unsigned long lastBounceMs = 0;

void setup() {
  Serial.begin(9600);
  pinMode(switchPin, INPUT_PULLUP);

  servoAngle = SERVO_MIN_ANGLE;
  liftServo.attach(servoPin);
  liftServo.write(servoAngle); // start lowered/at rest
}

void loop() {
  unsigned long now = millis();

  bool raw = digitalRead(switchPin);
  if (raw != lastRawState) {
    lastBounceMs = now;
    lastRawState = raw;
  }
  if (now - lastBounceMs > DEBOUNCE_MS && raw != debouncedState) {
    debouncedState = raw;
    if (debouncedState == LOW) { // a real, debounced press (pull-up -> GND)
      liftRunning = !liftRunning;
      Serial.print("lift running: ");
      Serial.println(liftRunning);
    }
  }

  if (liftRunning) {
    if (now - lastServoStepMs >= SERVO_STEP_INTERVAL_MS) {
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
    liftServo.write(servoAngle); // return to rest as soon as it's toggled off
  }
}
