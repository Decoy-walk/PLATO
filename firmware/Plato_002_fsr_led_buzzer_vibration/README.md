# Plato_002_fsr_led_buzzer_vibration (Seeeduino XIAO, SAMD21)

Extends `Plato_001_fsr_led` (the circuit actually exhibited at Maker Faire
Tokyo 2026) with two more outputs, both triggered together when the FSR is
squeezed past a threshold:

- A buzzer (TMB12A05).
- A vibration-motor breakout module (driver built into the module) shaking
  hard enough to rattle the printed plastic structure against itself.

The FSR→LED brightness response is unchanged and stays continuously
proportional to force; the buzzer/vibration are a separate on/off layer on
top, since "buzz" and "rattle loudly" are discrete effects, not something
meaningfully swept by force the way LED brightness is.

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
   certainly why the buzzer doesn't respond.

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

For ON, `D8` is driven `OUTPUT` `LOW`, sinking current and completing the
circuit. For OFF, **`D8` is switched to `INPUT` (high-impedance)**, not
driven `HIGH` — driving it `HIGH` only leaves ~1.7V across the buzzer
(5V - 3.3V), and this particular buzzer's driver IC can *sustain*
oscillation at that leftover voltage even though it needs the full 4V+ to
*start* it, so a once-triggered buzzer never actually stopped when tried
with `HIGH` instead. Switching to `INPUT` removes the current path
entirely, which reliably silences it with no extra hardware. The firmware
does this (`pinMode(buzzerPin, OUTPUT/INPUT)` + `digitalWrite(buzzerPin, LOW)`
for ON, not `tone()`/`noTone()` and not a plain `HIGH` for OFF).

The buzzer draws ~30mA when on. Driving that directly through a GPIO pin as
a sink is common practice for a small load like this. If you have a small
NPN transistor (e.g. 2N3904) on hand, using it as a true low-side switch
(`D8` → base through a ~1kΩ resistor, buzzer `-` lead → collector, emitter
→ GND, control logic flipped back to `HIGH`=on) is an equally valid
alternative to the `INPUT`-for-off trick above and keeps current off the
MCU pin entirely — but isn't required to fix the "won't turn off" symptom.

## What this doesn't do yet: Chladni/Cymatics patterns

The original three-part plan also called for mapping the sound to visible
Chladni-figure/Cymatics patterns. Neither this active buzzer (fixed
~2.4kHz, no frequency sweep possible) nor the ERM vibration motor has the
frequency control or power to drive visible nodal-line patterns in sand on
a plate — a real Chladni setup needs an amplified, frequency-swept signal
into a rigidly-mounted plate via a proper exciter/bass shaker. This build
treats the buzzer+vibration as a conceptual/experiential stand-in (press →
hear + feel + hear-the-structure-rattle) rather than a literal Chladni
demo. If a real visual pattern is wanted later, that's a separate hardware
addition (audio amp + exciter, or a software visualization driven by a mic
input) — flag it as its own task rather than folding it into this sketch.

## Wiring (no-rail mini breadboard)

Builds on the `Plato_001_fsr_led` breadboard (FSR on A0, LED on D9, rows
A-G — see that folder's README). This board has no shared power rails, so
every use of a supply voltage is its own point-to-point jumper back to its
source:

14. `D8` --jumper--> row H
15. Buzzer `-` lead (**long** lead) → row H
16. Buzzer `+` lead (**short** lead) → row I
17. XIAO `5V` pin --jumper--> row I
18. `D2` --jumper--> row J
19. Vibration module `IN` → row J
20. `3.3V-OUT` --jumper--> row K
21. Vibration module `VCC` → row K
22. `GND` --jumper--> row L
23. Vibration module `GND` → row L

**Buzzer** (TMB12A05, active, 2 leads): `+` (short) lead → row I (→ XIAO
`5V` pin), `-` (long) lead → row H (→ `D8`, `OUTPUT LOW`/`INPUT` switching — see above).

**Vibration motor module** (3-pin breakout, driver already on the board):
`D2` → `IN` (row J), `3V3` → `VCC` (row K, module is commonly rated 3-5V;
expect less punch at 3.3V than at 5V — try raising `VIBRATION_INTENSITY`
towards 255 first before reaching for a 5V source), `GND` → `GND` (row L).

## Wiring (standard breadboard with +/- power rails)

If you're not constrained to the no-rail mini board, a standard breadboard
with power rails is simpler to build: the chip needs only **one** jumper to
the +rail and **one** to the -rail total (plus the buzzer's dedicated 5V
jumper below, since that's a different voltage than the +rail), and every
other component taps a rail directly instead of getting its own dedicated
wire back to the chip. See `hardware/cad/plato_002_wiring_diagram_shared_rail.dxf`
for the layout, which includes the buzzer's dedicated 5V feed.

1. `3.3V-OUT` --jumper--> breadboard **+rail**
2. `GND` --jumper--> breadboard **-rail**
3. FSR402 leg 1 → +rail; leg 2 → node C
4. 1kΩ resistor leg 1 → node C; leg 2 → -rail
5. `A0` --jumper--> node C (the divider tap point)
6. `D9` → 330Ω resistor leg 1; leg 2 → LED anode; LED cathode → -rail
7. XIAO `5V` pin → buzzer `+` lead (**short**); buzzer `-` lead (**long**) → `D8`
8. `D2` → vibration module `IN`
9. Vibration module `VCC` → +rail
10. Vibration module `GND` → -rail

## Tuning

- `PRESS_ON_BRIGHTNESS` / `PRESS_OFF_BRIGHTNESS` (40 / 20): hysteresis
  thresholds on the same 0-255 brightness scale as the LED — widen the gap
  if the buzzer/vibration chatter on and off near the threshold.
- `VIBRATION_INTENSITY` (220): PWM duty to the vibration module's `IN` pin;
  raise towards 255 if the rattle effect feels weak.

## Setup / build / flash

Same as `Plato_001_fsr_led` — see that folder's README (Seeeduino:samd
board package, no external libraries).
