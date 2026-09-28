// Plato_003_fsr_led_buzzer.ino
//
// Extends Plato_001_fsr_led (the circuit exhibited at Maker Faire Tokyo
// 2026) with one more output, triggered when the FSR is pressed past a
// threshold: a buzzer. Same as Plato_002_fsr_led_buzzer_vibration but
// without the vibration motor module.
//
// The FSR->LED brightness mapping is unchanged from Plato_001 and stays
// continuously proportional to force; the buzzer is a separate on/off
// layer on top of that.
//
// Board: Tools > Board > Seeed SAMD Boards > Seeeduino XIAO
//
// Wiring (adds to the Plato_001_fsr_led breadboard - see that folder's
// README for the FSR/LED rows):
//   Buzzer - TMB12A05 (2-lead ACTIVE buzzer, built-in driver, fixed
//   ~2.4kHz internal tone, rated 4-8V/5V). Two things this is NOT: it is
//   not a passive piezo (tone()'s frequency argument can't set its pitch -
//   it always sounds at its own fixed ~2.4kHz), and it does not run off
//   the 3.3V rail (below its 4V minimum, so it likely won't oscillate at
//   all). Since the MCU's GPIO HIGH is only 3.3V, it can't source the
//   buzzer's required 5V either, so it's wired as a low-side switch instead:
//     XIAO "5V" pin -> buzzer + lead   (SHORT lead - opposite of the LED
//                                        long-leg-is-anode convention;
//                                        needs the board powered over USB,
//                                        which is where that pin gets 5V)
//     buzzer - lead -> D8              (LONG lead; GPIO sinks current to
//                                        switch it on)
//   Control logic: D8 driven LOW (OUTPUT) sinks current, completing the
//   circuit (buzzer ON). For OFF, D8 is switched to INPUT (high-impedance)
//   rather than driven HIGH - driving it HIGH only leaves ~1.7V across the
//   buzzer (5V - 3.3V), and some active-buzzer driver ICs can *sustain*
//   oscillation at that leftover voltage even though they need the full 4V+
//   to *start* it, so a once-triggered buzzer never actually stops. Setting
//   D8 to INPUT removes the current path entirely (no meaningful voltage
//   across the buzzer at all), which reliably silences it without needing
//   an extra transistor as a true low-side switch.

const int fsrPin = A0;
const int ledPin = 9;
const int buzzerPin = 8;

const float R_FIXED = 1000.0; // 1k ohm
const float VCC = 3.3;        // XIAO SAMD21 runs its ADC/IO at 3.3V

// FSR402 conductance range observed on the bench; re-measure per physical
// build (see Plato_001_fsr_led/README.md's calibration section).
float condMin = 0.000007;
float condMax = 0.001;

const int PRESS_ON_BRIGHTNESS = 40;  // brightness (0-255) above which the buzzer turns on
const int PRESS_OFF_BRIGHTNESS = 20; // below which it turns back off
// (the gap between ON/OFF is hysteresis, so sensor noise right at the
// threshold can't make it chatter on and off rapidly)

bool pressed = false;

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(fsrPin, INPUT);
  pinMode(buzzerPin, INPUT); // high-impedance = buzzer off (see wiring note above)
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
    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW); // sinks current, completing the 5V-fed buzzer's circuit
  } else {
    pinMode(buzzerPin, INPUT); // high-impedance: no current path at all, so it's reliably silent
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
