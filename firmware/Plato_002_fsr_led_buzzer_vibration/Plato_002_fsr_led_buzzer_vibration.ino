// Plato_002_fsr_led_buzzer_vibration.ino
//
// Extends Plato_001_fsr_led (the circuit exhibited at Maker Faire Tokyo
// 2026) with two more outputs, both triggered together when the FSR is
// pressed past a threshold: a buzzer, and a vibration-motor breakout
// module (driver built into the module, so its IN pin can be driven
// directly from a GPIO/PWM pin) shaking hard enough to rattle the printed
// plastic structure against itself.
//
// The FSR->LED brightness mapping is unchanged from Plato_001 and stays
// continuously proportional to force; the buzzer and vibration motor are
// a separate on/off layer on top of that.
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
//   all - this is the most likely reason the buzzer didn't work). Since
//   the MCU's GPIO HIGH is only 3.3V, it can't source the buzzer's
//   required 5V either, so it's wired as a low-side switch instead:
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
//
//   Since this buzzer's ~2.4kHz tone/timbre is fixed in hardware and can't
//   be changed by software, "sounds harsh" is addressed by shaping the
//   on/off *rhythm* instead: rather than one continuous drone for the
//   whole press, the buzzer is pulsed into short chirps (BUZZ_ON_MS on,
//   BUZZ_OFF_MS off, repeating) while pressed, which reads as a soft
//   chirping/beeping pattern instead of a harsh flat tone. Each chirp's
//   OFF gap still goes through the same INPUT high-impedance trick above
//   (not HIGH), so it stays reliably silent between chirps too - PWMing
//   the sink pin instead (partial duty) was considered and rejected,
//   since the leftover voltage during the HIGH portion of a PWM cycle can
//   re-trigger the same "won't turn off" sustain behavior described above.
//   Tune BUZZ_ON_MS / BUZZ_OFF_MS to taste (shorter ON / longer OFF reads
//   as a lighter "tick"; longer ON / shorter OFF approaches a continuous
//   drone again). For a genuinely different pitch (e.g. closer to 600Hz)
//   or a smoother waveform, this active buzzer would need to be swapped
//   for a passive piezo (no built-in driver) driven with tone() - see this
//   folder's README.
//
//   Vibration motor module (3-pin breakout, driver already on the board):
//     D2 -> IN
//     3V3 -> VCC   (module is commonly rated 3-5V; expect a bit less punch
//                    at 3.3V than at 5V - bump VIBRATION_INTENSITY towards
//                    255 first if it feels weak before reaching for 5V)
//     GND -> GND

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

const int PRESS_ON_BRIGHTNESS = 40;  // brightness (0-255) above which the buzzer+vibration turn on
const int PRESS_OFF_BRIGHTNESS = 20; // below which they turn back off
// (the gap between ON/OFF is hysteresis, so sensor noise right at the
// threshold can't make them chatter on and off rapidly)
const int VIBRATION_INTENSITY = 220; // 0-255 PWM to the module's IN pin

const unsigned long SENSE_INTERVAL_MS = 50; // FSR read / LED update rate
// A too-short ON pulse cuts the buzzer off before its internal driver
// finishes ramping up to full volume, and a low ON/(ON+OFF) duty cycle
// lowers the averaged loudness on top of that - both made the first chirp
// version (25/70) noticeably quieter than the old continuous tone. 90/35
// keeps an audible pulsing texture (not a flat drone) while giving the
// driver enough ON time to reach full volume and raising the duty back up.
const unsigned long BUZZ_ON_MS = 90;  // each chirp's ON duration
const unsigned long BUZZ_OFF_MS = 35; // gap between chirps while pressed

bool pressed = false;
bool buzzChirpOn = false;
unsigned long lastSenseMs = 0;
unsigned long lastBuzzToggleMs = 0;

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(fsrPin, INPUT);
  pinMode(buzzerPin, INPUT); // high-impedance = buzzer off (see wiring note above)
  pinMode(vibrationPin, OUTPUT);
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

    int brightness = (int)((conductance - condMin) / (condMax - condMin) * 255);
    brightness = constrain(brightness, 0, 255);
    analogWrite(ledPin, brightness);

    if (!pressed && brightness > PRESS_ON_BRIGHTNESS) {
      pressed = true;
    } else if (pressed && brightness < PRESS_OFF_BRIGHTNESS) {
      pressed = false;
    }

    analogWrite(vibrationPin, pressed ? VIBRATION_INTENSITY : 0);

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
}
