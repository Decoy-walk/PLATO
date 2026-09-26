// Plato_002_fsr_led_buzzer_vibration.ino
//
// Extends Plato_001_fsr_led (the circuit exhibited at Maker Faire Tokyo
// 2026) with two more outputs, both triggered together when the FSR is
// pressed past a threshold: a passive piezo buzzer at a fixed 600 Hz tone,
// and a vibration-motor breakout module (driver built into the module, so
// its IN pin can be driven directly from a GPIO/PWM pin) shaking hard
// enough to rattle the printed plastic structure against itself.
//
// The FSR->LED brightness mapping is unchanged from Plato_001 and stays
// continuously proportional to force; the buzzer and vibration motor are
// a separate on/off layer on top of that, since a fixed 600 Hz tone and a
// "make it rattle loudly" vibration are discrete effects, not something
// meaningfully swept by force the way LED brightness is.
//
// Board: Tools > Board > Seeed SAMD Boards > Seeeduino XIAO
//
// Wiring (adds to the Plato_001_fsr_led breadboard - see that folder's
// README for the FSR/LED rows):
//   Buzzer (passive piezo, 2 leads):
//     D8 -> buzzer leg 1
//     GND -> buzzer leg 2
//   Vibration motor module (3-pin breakout, driver already on the board):
//     D2 -> IN
//     3V3 -> VCC   (module is commonly rated 3-5V; expect a bit less punch
//                    at 3.3V than at 5V - bump VIBRATION_INTENSITY towards
//                    255 first if it feels weak before reaching for 5V)
//     GND -> GND
//
// Note: tone() takes over a hardware timer on SAMD21 for its duration. If
// driving the buzzer visibly disturbs the LED's brightness (flicker/dimming
// while the tone plays), it means D8 and D9 share a timer on this board -
// move the buzzer to a different pin and re-test.

const int fsrPin = A0;
const int ledPin = 9;
const int buzzerPin = 8;
const int vibrationPin = 2;

const float R_FIXED = 1000.0; // 1k ohm
const float VCC = 3.3;        // XIAO SAMD21 runs its ADC/IO at 3.3V

// FSR402 conductance range observed on the bench; re-measure per physical
// build (see Plato_001_fsr_led/README.md's calibration section).
float condMin = 0.000007;
float condMax = 0.001;

const int BUZZER_TONE_HZ = 600;
const int PRESS_ON_BRIGHTNESS = 40;  // brightness (0-255) above which the buzzer+vibration turn on
const int PRESS_OFF_BRIGHTNESS = 20; // below which they turn back off
// (the gap between ON/OFF is hysteresis, so sensor noise right at the
// threshold can't make them chatter on and off rapidly)
const int VIBRATION_INTENSITY = 220; // 0-255 PWM to the module's IN pin

bool pressed = false;

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(fsrPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(vibrationPin, OUTPUT);
}

void loop() {
  int raw = analogRead(fsrPin);
  float vOut = raw * (VCC / 1023.0);

  float conductance;
  if (vOut <= 0.01) {
    conductance = 0;
  } else {
    float rFsr = R_FIXED * (VCC - vOut) / vOut;
    conductance = 1.0 / rFsr;
  }

  int brightness = (int)((conductance - condMin) / (condMax - condMin) * 255);
  brightness = constrain(brightness, 0, 255);
  analogWrite(ledPin, brightness);

  if (!pressed && brightness > PRESS_ON_BRIGHTNESS) {
    pressed = true;
  } else if (pressed && brightness < PRESS_OFF_BRIGHTNESS) {
    pressed = false;
  }

  if (pressed) {
    tone(buzzerPin, BUZZER_TONE_HZ);
    analogWrite(vibrationPin, VIBRATION_INTENSITY);
  } else {
    noTone(buzzerPin);
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

  delay(50);
}
