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

## Wiring

Builds on the `Plato_001_fsr_led` breadboard (FSR on A0, LED on D9 — see
that folder's README for those rows). Adds:

**Buzzer** (passive piezo, 2 leads):
- `D8` → buzzer leg 1
- `GND` → buzzer leg 2

**Vibration motor module** (3-pin breakout, driver already on the board):
- `D2` → `IN`
- `3V3` → `VCC` (module is commonly rated 3-5V; expect less punch at 3.3V
  than at 5V — try raising `VIBRATION_INTENSITY` towards 255 first before
  reaching for a 5V source)
- `GND` → `GND`

> `tone()` takes over a hardware timer on SAMD21 for its duration. If
> playing the buzzer visibly disturbs the LED's brightness (flicker/dimming
> while the tone plays), `D8` and `D9` share a timer on this board — move
> the buzzer to a different pin and re-test.

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
