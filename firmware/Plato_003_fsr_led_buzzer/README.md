# Plato_003_fsr_led_buzzer (Seeeduino XIAO, SAMD21)

Extends `Plato_001_fsr_led` (the circuit actually exhibited at Maker Faire
Tokyo 2026) with one more output, triggered when the FSR is squeezed past
a threshold: a buzzer (TMB12A05). Same as
`Plato_002_fsr_led_buzzer_vibration` but **without the vibration motor
module** — use this if you only want sound feedback.

The FSR→LED brightness response is unchanged and stays continuously
proportional to force; the buzzer is a separate on/off layer on top, since
"buzz" is a discrete effect, not something meaningfully swept by force the
way LED brightness is.

## The buzzer is an active buzzer, not a passive piezo

TMB12A05 has a built-in driver and a fixed ~2.4kHz internal tone, rated
**4-8V (5V nominal)**. Two consequences that weren't obvious from the part
just looking like "a buzzer":

1. **Its pitch isn't settable.** `tone(pin, frequency)` only works on a bare
   passive piezo element; on an active buzzer like this one, any signal
   that turns it on just makes it sound at its own fixed ~2.4kHz — you
   can't dial in an arbitrary frequency.
2. **It needs 5V, not 3.3V, to reliably start.** Below its ~4V minimum, the
   internal driver likely won't oscillate at all — silence, not a quiet
   tone. Wiring its `+` lead to the XIAO's 3.3V rail (the natural thing to
   do next to the FSR/LED circuit, which does run at 3.3V) is almost
   certainly why a buzzer wired that way doesn't respond.

Since the MCU's GPIO HIGH is only 3.3V, it can't source the required 5V
either, so this build wires the buzzer as a **low-side switch**:

```
XIAO "5V" pin --------> buzzer + lead
buzzer - lead --------> D8
```

(The `5V` pin sources USB's raw 5V when the board is powered over USB —
this wiring only works while USB-powered, not on battery/regulated-3.3V-only
power.)

> **Lead identification: the SHORT lead is `+`, the LONG lead is `-`** —
> backwards from the LED long-leg-is-anode convention, easy to get wrong by
> habit. A printed `+` on the case can also fall off or be inconsistently
> placed, so trust lead length over any marking. Reversed polarity can
> prevent the buzzer from working at all.

With this wiring the control logic is **active-low**: `D8` LOW sinks
current and completes the circuit (buzzer ON); `D8` HIGH leaves only 3.3V
across the buzzer, below its minimum, so it goes silent (OFF). The firmware
already does this (`digitalWrite(buzzerPin, LOW/HIGH)`, not `tone()`/`noTone()`).

The buzzer draws ~30mA when on. Driving that directly through a GPIO pin as
a sink is common practice for a small load like this, but if you'd rather
not run current straight through the MCU pin, add a small NPN transistor
(e.g. 2N3904) as the low-side switch instead: `D8` → base (through a ~1kΩ
resistor), buzzer `-` lead → collector, emitter → GND.

## Wiring (no-rail mini breadboard)

Builds on the `Plato_001_fsr_led` breadboard (FSR on A0, LED on D9, rows
A-G — see that folder's README). This board has no shared power rails, so
every use of a supply voltage is its own point-to-point jumper back to its
source:

14. `D8` --jumper--> row H
15. Buzzer `-` lead (**long** lead) → row H
16. Buzzer `+` lead (**short** lead) → row I
17. XIAO `5V` pin --jumper--> row I

**Buzzer** (TMB12A05, active, 2 leads): `+` (short) lead → row I (→ XIAO
`5V` pin), `-` (long) lead → row H (→ `D8`, active-low switching — see above).

## Wiring (standard breadboard with +/- power rails)

If you're not constrained to the no-rail mini board: the chip needs only
**one** jumper to the +rail and **one** to the -rail total for the FSR/LED
circuit (plus the buzzer's dedicated 5V jumper, since that's a different
voltage than the +rail), and every other component taps a rail directly
instead of getting its own dedicated wire back to the chip.

1. `3.3V-OUT` --jumper--> breadboard **+rail**
2. `GND` --jumper--> breadboard **-rail**
3. FSR402 leg 1 → +rail; leg 2 → node C
4. 1kΩ resistor leg 1 → node C; leg 2 → -rail
5. `A0` --jumper--> node C (the divider tap point)
6. `D9` → 330Ω resistor leg 1; leg 2 → LED anode; LED cathode → -rail
7. XIAO `5V` pin → buzzer `+` lead (**short**); buzzer `-` lead (**long**) → `D8`

## Tuning

- `PRESS_ON_BRIGHTNESS` / `PRESS_OFF_BRIGHTNESS` (40 / 20): hysteresis
  thresholds on the same 0-255 brightness scale as the LED — widen the gap
  if the buzzer chatters on and off near the threshold.
- `condMin` / `condMax`: FSR402 conductance range, re-measure per physical
  build (see `Plato_001_fsr_led/README.md`'s calibration section).

## Setup / build / flash

Same as `Plato_001_fsr_led` — see that folder's README (Seeeduino:samd
board package, no external libraries).

## Adding the vibration module back

See `firmware/Plato_002_fsr_led_buzzer_vibration/` — same circuit plus a
3-pin vibration motor module on `D2`.
