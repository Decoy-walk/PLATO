# Plato_004_fsr_led_buzzer_vibration_lift (Seeeduino XIAO, SAMD21)

Extends `Plato_002_fsr_led_buzzer_vibration` with a fourth output: a
servo-driven **slider-crank mechanism** that physically lifts and lowers
the resonance plate while pressed, instead of only buzzing/vibrating it.
FSR→LED brightness, the buzzer's chirp pattern, and the vibration motor's
force-proportional intensity are all unchanged — see
`Plato_002_fsr_led_buzzer_vibration/README.md` for the reasoning behind
each of those. This README only covers the new lift layer.

## Why a slider-crank, and why it works with only a servo

The only actuator available for this (`SFE-ROB-14760`) is a hobby **servo**,
not a continuous-rotation motor — it sweeps across a limited arc (rated
~160°), it doesn't spin all the way around. That's not actually a problem
for "rotary motion → up/down motion": a slider-crank (the same mechanism
that converts a piston engine's crank rotation into linear motion) only
needs its crank pin to sweep through *some* arc to make the slider
reciprocate once per sweep — full 360° rotation was never required, just
like a windshield-wiper linkage.

```
        servo horn (crank)
             o────┐
            /      \  crank pin (off-center, ~10mm radius)
    servo  ⊙        \
   shaft            ◇ ── connecting rod (~40mm) ── [slider] ↕ (vertical guide)
                                                        │
                                                   resonance plate
```

- **Servo horn = crank.** A pin/screw mounted off-center on the horn
  (crank radius — start around **10mm**) traces an arc as the servo sweeps
  between `SERVO_MIN_ANGLE` and `SERVO_MAX_ANGLE`.
- **Connecting rod** (start around **40mm**, roughly 3-4× the crank
  radius so it doesn't bind near the sweep limits) links that pin to the
  slider with simple pin joints at both ends.
- **The slider must be constrained to vertical-only motion by its own
  guide** — two vertical printed rails/rods through sleeves on the
  slider, or a drawer-slide. The guide is what keeps the slider from
  swinging sideways with the crank; the connecting rod only needs to pivot
  freely at both ends.
- **The resonance plate sits on/attaches to the slider.**

Start with `SERVO_MIN_ANGLE`/`SERVO_MAX_ANGLE` narrower than the servo's
full range (default **20°–140°**, inside its ~160° rated travel) so the
linkage never slams into its own mechanical end stops. Widen gradually
once the real geometry (rod length, pin placement, guide friction) is
tuned on the bench — crank radius, rod length, and the angle range are all
physical-build parameters to dial in by testing, not something to get
exactly right on paper first.

## Wiring (adds to the Plato_002 breadboard)

Builds on `Plato_002_fsr_led_buzzer_vibration`'s wiring (FSR/LED/buzzer/
vibration rows — see that folder's README):

- `D1` → servo signal wire (yellow/white)
- XIAO `5V` pin → servo power wire (red)
- `GND` → servo ground wire (brown)

**Why the `5V` pin again:** `ROB-14760` is rated **4.8-6V**, the same
reason the buzzer needed the `5V` pin instead of `3V3` — see Plato_002's
README. This needs the board USB-powered (the `5V` pin is a raw USB VBUS
passthrough, not available on battery/3.3V-only power).

**Why the signal wire doesn't need level-shifting:** the servo is powered
at 5V, but its signal/pulse-decoder input still reads the XIAO's 3.3V
logic level as a valid HIGH — practically all hobby servos' decoder ICs
recognize anything above roughly 2-2.5V as HIGH, well below 3.3V. Only the
power wire needs the `5V` pin; the signal wire connects straight to `D1`.

**Why `D1` and not `D6`/`D7`:** the Seeed SAMD core's `Servo` library port
has a known timer conflict with `analogWrite()` specifically on pins 6 and
7 (unrelated to any pin already used in this build, but worth avoiding if
you relocate other outputs later).

## Tuning

- `SERVO_MIN_ANGLE` / `SERVO_MAX_ANGLE` (20 / 140): the plate's lowered/
  raised servo angles. Narrow this range first if the linkage binds or
  the plate doesn't travel smoothly through the whole sweep; widen it
  (towards, but not past, the servo's full ~160° rating) once the
  mechanism is tuned, for a longer stroke.
- `SERVO_STEP_DEGREES` (2): how far the servo moves per internal step —
  larger steps feel jerkier/more mechanical, smaller feel smoother but
  need more steps (and therefore shorter `SERVO_STEP_INTERVAL_*_MS`) to
  reach the same speed.
- `SERVO_STEP_INTERVAL_MIN_MS` / `_MAX_MS` (8 / 30): like the vibration
  motor, lift speed scales with press force — a light press raises/lowers
  slowly, a hard press moves quickly. Lower `_MIN_MS` for a snappier fast
  end; raise `_MAX_MS` for a gentler slow end.

## Setup / build / flash

Same as `Plato_001_fsr_led` — see that folder's README (Seeeduino:samd
board package). This sketch additionally needs the standard Arduino
`Servo` library (Library Manager → "Servo" by Arduino — it's one of the
default libraries bundled with most Arduino IDE installs, not a separate
install in most cases).
