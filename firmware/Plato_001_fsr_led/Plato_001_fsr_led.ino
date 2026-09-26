// Plato_001_fsr_led.ino
//
// Actual circuit exhibited at Maker Faire Tokyo 2026 (Sep 4-8) as part of
// the "E. Modular" piece (a FidgetSqueezer-style folding ball, see
// hardware/cad/ and hardware/docs/MFT26_devlog.pdf. One FSR402 reads how hard
// the ball is being squeezed at one joint; one LED brightens in response.
// This is a separate, minimal build from firmware/PLATO_XIAO_SAMD21/ (the
// 20-hinge research firmware) - different physical object, different
// purpose.
//
// Board: Tools > Board > Seeed SAMD Boards > Seeeduino XIAO
//
// Wiring (no-rail mini breadboard, XIAO SAMD21 straddling the middle):
//   FSR circuit (chip's left side, A0):
//     3.3V-OUT (chip right side) --jumper--> row B
//     FSR402 leg 1 -> row B (same row as the 3.3V jumper)
//     FSR402 leg 2 -> row C
//     1k ohm resistor leg 1 -> row C (voltage-divider junction)
//     1k ohm resistor leg 2 -> row D
//     GND (chip right side) --jumper--> row D
//     A0 (chip left side) --jumper--> row C  (the divider tap point)
//   LED circuit (chip's right side, D9):
//     D9 --jumper--> row E
//     330 ohm resistor leg 1 -> row E
//     330 ohm resistor leg 2 -> row F
//     LED anode (long leg) -> row F
//     LED cathode (short leg) -> row G
//     GND --jumper--> row G

const int fsrPin = A0;
const int ledPin = 9;

const float R_FIXED = 1000.0; // 1k ohm
const float VCC = 3.3;        // XIAO SAMD21 runs its ADC/IO at 3.3V

// FSR402 conductance range observed on the bench; condMax in particular
// should be re-measured for each physical build (grip strength, mounting
// pressure, and the specific FSR sample all shift it) - see the printed
// conductance value over serial while squeezing at full force.
float condMin = 0.000007;
float condMax = 0.001; // measure and adjust after bench testing

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(fsrPin, INPUT);
}

void loop() {
  int raw = analogRead(fsrPin);
  float vOut = raw * (VCC / 1023.0);

  // FSR402 is a variable resistor in a voltage divider with R_FIXED;
  // convert the divider's output voltage back to resistance, then to
  // conductance (1/R) since conductance responds roughly linearly to
  // applied force, unlike the resistance itself.
  float conductance;
  if (vOut <= 0.01) {
    conductance = 0; // essentially no force -> avoid dividing by ~0
  } else {
    float rFsr = R_FIXED * (VCC - vOut) / vOut;
    conductance = 1.0 / rFsr;
  }

  int brightness = (int)((conductance - condMin) / (condMax - condMin) * 255);
  brightness = constrain(brightness, 0, 255);
  analogWrite(ledPin, brightness);

  Serial.print("raw: ");
  Serial.print(raw);
  Serial.print("\tconductance: ");
  Serial.print(conductance, 6);
  Serial.print("\tbrightness: ");
  Serial.println(brightness);

  delay(50);
}
