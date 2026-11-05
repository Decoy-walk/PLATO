# Plato_005_tact_lift (Seeeduino XIAO, SAMD21)

A simplified, standalone build on a **fresh breadboard**: a tact switch
replaces the FSR402 as the trigger, and the servo slider-crank lift
mechanism (from `Plato_004_fsr_led_buzzer_vibration_lift`) is the only
output — no LED, buzzer, or vibration motor here. **Pressing the switch
toggles the lift on/off**: press once and it starts cycling up and down
continuously on its own; press again and it stops, returning to its
lowered rest position.

The mechanism itself (crank, connecting rod, vertical guide, slider,
plate) is unchanged from `Plato_004` — see that folder's README and
`hardware/cad/plato_004_lift_parts.dxf` for the physical build. Only the
control board/firmware is different here.

## Why a toggle instead of "hold to run"

The FSR gave a continuous analog force value, so Plato_002/004 could
scale the LED/buzzer/vibration/lift continuously with how hard you
pressed, and run only while actively held. A tact switch is purely
digital (pressed or not) — there's no force signal left to scale
anything by, and requiring it to be held down for the whole lift cycle
would fight the "press it and it does its thing automatically" feel this
build is going for. So instead: a (debounced) press flips a `liftRunning`
flag, and the lift free-runs at a fixed speed until the next press flips
it off again.

## Wiring (new breadboard, just two components)

- **Tact switch**: one leg → `D2`, other leg → `GND`.
  Read with `INPUT_PULLUP` — `D2` idles `HIGH` internally, and pressing
  the switch connects it straight to `GND`, pulling it `LOW`. No external
  pull-up/pull-down resistor needed.
- **Servo** (`ROB-14760`, 3-wire):
  - `D1` → signal (yellow/white)
  - XIAO `5V` pin → power (red) — rated 4.8-6V, needs the raw USB VBUS
    `5V` pin, not `3V3` (same reasoning as the buzzer in `Plato_002` and
    the servo in `Plato_004`). Needs the board USB-powered.
  - `GND` → ground (brown)

## Debouncing

A tact switch's contacts physically "bounce" for a few ms on each press
and release — read fast enough, a microcontroller sees several rapid
HIGH/LOW transitions instead of one clean edge. Without debouncing, a
single physical press could toggle the lift on and off multiple times in
a row unpredictably. `DEBOUNCE_MS` (30ms default) requires a reading to
stay stable for that long before a transition counts as a real press —
standard practice for any mechanical switch, not specific to this build.

## Tuning

- `SERVO_MIN_ANGLE` / `SERVO_MAX_ANGLE` (20 / 140): same meaning as in
  `Plato_004` — the lowered/raised servo angles. Keep these matched to
  whatever you already tuned the physical linkage to.
- `SERVO_STEP_DEGREES` (2) / `SERVO_STEP_INTERVAL_MS` (15): the lift's
  sweep speed. Unlike `Plato_004`, this is a single fixed speed (no force
  signal to grade it by) — lower `SERVO_STEP_INTERVAL_MS` or raise
  `SERVO_STEP_DEGREES` for faster/more energetic cycling, the reverse for
  slower/gentler.
- `DEBOUNCE_MS` (30): raise it if presses still occasionally double-toggle;
  most tact switches settle well under 30ms, so this rarely needs touching.

## Setup / build / flash

Board package: Seeeduino:samd (same as every other build in this repo —
see `Plato_001_fsr_led/README.md` for the one-time board manager setup).
This sketch needs the standard Arduino `Servo` library (Library Manager →
"Servo" by Arduino — usually bundled already, same as `Plato_004`).

For the physical slider-crank assembly (3D-printed parts, drilling the
crank pin into the servo horn, linkage assembly order), follow
`Plato_004_fsr_led_buzzer_vibration_lift/README.md`'s "How to build the
mechanism" and "Full build order" sections — everything there about the
mechanism itself still applies; just swap its "Wiring" and firmware steps
for this file's instead.
