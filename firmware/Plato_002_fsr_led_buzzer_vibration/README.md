# Plato_002_fsr_led_buzzer_vibration (Seeeduino XIAO, SAMD21)

Extends `Plato_001_fsr_led` (the circuit actually exhibited at Maker Faire
Tokyo 2026) with two more outputs, both triggered together when the FSR is
squeezed past a threshold:

- A passive piezo buzzer sounding a fixed 600 Hz tone.
- A vibration-motor breakout module (driver built into the module) shaking
  hard enough to rattle the printed plastic structure against itself.

The FSR→LED brightness response is unchanged and stays continuously
proportional to force; the buzzer/vibration are a separate on/off layer on
top, since "buzz at 600 Hz" and "rattle loudly" are discrete effects, not
something meaningfully swept by force the way LED brightness is.

## What this doesn't do yet: Chladni/Cymatics patterns

The original three-part plan also called for mapping the sound to visible
Chladni-figure/Cymatics patterns. A passive piezo buzzer and an ERM
vibration motor don't have the power or the frequency control to drive
visible nodal-line patterns in sand on a plate — a real Chladni setup needs
an amplified signal into a rigidly-mounted plate via a proper exciter/bass
shaker. This build treats the buzzer+vibration as a conceptual/experiential
stand-in (press → hear + feel + hear-the-structure-rattle) rather than a
literal Chladni demo. If a real visual pattern is wanted later, that's a
separate hardware addition (audio amp + exciter, or a software
visualization driven by a mic input) — flag it as its own task rather than
folding it into this sketch.

## Wiring (no-rail mini breadboard)

Builds on the `Plato_001_fsr_led` breadboard (FSR on A0, LED on D9, rows
A-G — see that folder's README). This board has no shared power rails, so
every use of 3V3/GND is its own point-to-point jumper back to the chip:

14. `D8` --jumper--> row H
15. Buzzer leg 1 → row H
16. Buzzer leg 2 → row I
17. `GND` --jumper--> row I
18. `D2` --jumper--> row J
19. Vibration module `IN` → row J
20. `3.3V-OUT` --jumper--> row K
21. Vibration module `VCC` → row K
22. `GND` --jumper--> row L
23. Vibration module `GND` → row L

**Buzzer** (passive piezo, 2 leads): `D8` → leg 1 (row H), `GND` → leg 2 (row I).

**Vibration motor module** (3-pin breakout, driver already on the board):
`D2` → `IN` (row J), `3V3` → `VCC` (row K, module is commonly rated 3-5V;
expect less punch at 3.3V than at 5V — try raising `VIBRATION_INTENSITY`
towards 255 first before reaching for a 5V source), `GND` → `GND` (row L).

> `tone()` takes over a hardware timer on SAMD21 for its duration. If
> playing the buzzer visibly disturbs the LED's brightness (flicker/dimming
> while the tone plays), `D8` and `D9` share a timer on this board — move
> the buzzer to a different pin and re-test.

## Wiring (standard breadboard with +/- power rails)

If you're not constrained to the no-rail mini board, a standard breadboard
with power rails is simpler to build: the chip needs only **one** jumper to
the +rail and **one** to the -rail total, and every component taps a rail
directly instead of getting its own dedicated wire back to the chip.
See `hardware/cad/plato_002_wiring_diagram_shared_rail.dxf` for the layout.

1. `3.3V-OUT` --jumper--> breadboard **+rail**
2. `GND` --jumper--> breadboard **-rail**
3. FSR402 leg 1 → +rail; leg 2 → node C
4. 1kΩ resistor leg 1 → node C; leg 2 → -rail
5. `A0` --jumper--> node C (the divider tap point)
6. `D9` → 330Ω resistor leg 1; leg 2 → LED anode; LED cathode → -rail
7. `D8` → buzzer leg 1; buzzer leg 2 → -rail
8. `D2` → vibration module `IN`
9. Vibration module `VCC` → +rail
10. Vibration module `GND` → -rail

## Tuning

- `BUZZER_TONE_HZ` (600): fixed tone frequency per the original plan.
- `PRESS_ON_BRIGHTNESS` / `PRESS_OFF_BRIGHTNESS` (40 / 20): hysteresis
  thresholds on the same 0-255 brightness scale as the LED — widen the gap
  if the buzzer/vibration chatter on and off near the threshold.
- `VIBRATION_INTENSITY` (220): PWM duty to the vibration module's `IN` pin;
  raise towards 255 if the rattle effect feels weak.

## Setup / build / flash

Same as `Plato_001_fsr_led` — see that folder's README (Seeeduino:samd
board package, no external libraries).
