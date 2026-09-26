# Plato_001_fsr_led (Seeeduino XIAO, SAMD21)

The actual circuit built and exhibited at Maker Faire Tokyo 2026
(Sep 4-8) as the interactive element of "E. Modular" — a FidgetSqueezer-
style folding ball assembled from 3D-printed hinge modules and rubber
bands (see `hardware/cad/` and `hardware/docs/MFT26_devlog.pdf`). One FSR402
at a joint reads squeeze force; one LED brightens in response. This is a
separate, minimal build from `firmware/PLATO_XIAO_SAMD21/` (the 20-hinge
research firmware) — different physical object, different purpose.

## Wiring

No-rail mini breadboard, XIAO SAMD21 straddling the middle:

**FSR circuit** (chip's left side, `A0`):

| Step | Connection |
|------|------------|
| 1 | Jumper: `3V3-OUT` (chip right side) → row B |
| 2 | FSR402 leg 1 → row B |
| 3 | FSR402 leg 2 → row C |
| 4 | 1kΩ resistor leg 1 → row C (voltage-divider junction) |
| 5 | 1kΩ resistor leg 2 → row D |
| 6 | Jumper: `GND` (chip right side) → row D |
| 7 | Jumper: `A0` (chip left side) → row C (the divider tap point) |

**LED circuit** (chip's right side, `D9`):

| Step | Connection |
|------|------------|
| 8 | Jumper: `D9` → row E |
| 9 | 330Ω resistor leg 1 → row E |
| 10 | 330Ω resistor leg 2 → row F |
| 11 | LED anode (long leg) → row F |
| 12 | LED cathode (short leg) → row G |
| 13 | Jumper: `GND` → row G |

Row C must contain exactly three things: FSR leg 2, the 1kΩ resistor's
first leg, and the A0 jumper — that's the voltage-divider tap point. Row B
(power) and row G (LED ground) each need their own separate 3.3V/GND
jumper, since this board has no shared power rails.

## Setup

```bash
arduino-cli config add board_manager.additional_urls \
  https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
arduino-cli core update-index
arduino-cli core install Seeeduino:samd
```

In Arduino IDE: Tools > Board > **Seeed SAMD Boards > Seeeduino XIAO**.

## Build & flash

```bash
arduino-cli compile --fqbn Seeeduino:samd:seeed_XIAO_m0 .
arduino-cli upload --fqbn Seeeduino:samd:seeed_XIAO_m0 -p /dev/ttyACM0 .
```

No external libraries — Arduino SAMD core only.

## Calibrating `condMin` / `condMax`

Open the serial monitor at 9600 baud. With nothing touching the FSR, note
the printed `conductance` (should be near `condMin`); squeeze at the
hardest force you expect at the exhibit and note that value for
`condMax`. Re-measure per physical build — grip strength, mounting
pressure, and the individual FSR sample all shift this range.
