# Bill of Materials — Plato_004_fsr_led_buzzer_vibration_lift

Everything needed to build the full stack (FSR → LED + buzzer + vibration
+ servo lift). `Plato_003`/`Plato_005` need only a subset — see the "Which
builds need this" column.

## Electronics

| Part | Qty | Spec | Used in | Notes |
|------|-----|------|---------|-------|
| Seeed XIAO SAMD21 | 1 | — | All builds | MCU board (Tools > Board > Seeeduino XIAO) |
| FSR402 | 1 | Force-sensitive resistor | 001/002/003/004 (not 005) | Squeeze-force sensing |
| Resistor | 1 | 1kΩ | 001/002/003/004 | FSR voltage-divider fixed leg |
| Resistor | 1 | 330Ω | 001/002/003/004 | LED current-limiting |
| LED | 1 | Any standard 3-5mm | 001/002/003/004 | Brightness ∝ press force |
| TMB12A05 | 1 | Active buzzer, 5V, fixed ~2.4kHz | 002/004 (not 003) | Needs XIAO `5V` pin, not `3V3` — see `Plato_002` README |
| Vibration motor module | 1 | ERM, 3-pin breakout, driver onboard | 002/004 | Graded with press force |
| Servo | 1 | `SFE-ROB-14760` (SparkFun, metal gear micro, 4.8-6V) | 004/005 | Drives the slider-crank lift |
| Tact switch | 1 | Momentary pushbutton | 005 only | Replaces the FSR in `Plato_005` |
| Breadboard | 1 | No-rail mini OR standard w/ +/- rails | All builds | See `hardware/cad/` for the two wiring diagram variants |
| Jumper wires | ~15-20 | M-M (+ a few M-F for the servo/buzzer leads if using header cables) | All builds | No-rail boards need more (point-to-point); shared-rail needs fewer |
| USB-C cable | 1 | Data-capable (not charge-only) | All builds | Needed for both flashing and the `5V` pin's power (buzzer/servo need USB power, not just a battery) |

## 3D-printed parts

From `hardware/cad/plato_004_lift_parts.dxf` (Fusion 360: Insert > Insert
DXF, one profile per sketch, extrude per the thickness below):

| Part | Qty | Extrude thickness | Key dimensions |
|------|-----|--------------------|-----------------|
| Connecting rod | 1 | ~3-4mm | 40mm hole-to-hole, ⌀3.2mm holes |
| Slider block | 1 | ~10mm | 36x20mm, rail holes ⌀4.3mm (28mm apart), pivot hole ⌀3.2mm, plate holes ⌀3.2mm (20mm apart) |
| Base bracket | 1 | ~10mm | 50x30mm, rail holes ⌀4.3mm (28mm apart), servo slot 29.5x14.1mm (dry-fit — print tolerance varies) |

The servo horn itself is **not** printed — use the one that ships with the
`ROB-14760` (drilled per `crank_pin_drill_diagram.png`).

## Mechanical hardware

| Part | Qty | Spec | Used for |
|------|-----|------|----------|
| Guide rod | 2 | ⌀4mm, ~60-70mm long (steel/brass smoother than printed) | Slider's vertical travel |
| M3 screw | 1 | ~10-12mm long | Crank pin (through the drilled horn) |
| M3 nut | 2 | — | Crank pin's lock nut + retaining nut |
| M3 screw | 1 | ~10-12mm long | Slider pivot (connecting rod's other end) |
| M3 nut | 2 | — | Slider pivot's lock nut + retaining nut |
| M3 screw | 2 | Length depends on plate thickness | Plate mounting — **only if** using the screwed-on option |
| M3 heat-set insert (or 2 nuts) | 2 | — | Plate mounting — **only if** using the screwed-on option and the plate takes inserts |

Total M3 screws: 4 minimum (crank pin + slider pivot), up to 6 if the
plate is screwed on. Total M3 nuts: 4 minimum, up to 6 with screwed-on
plate (or 0 extra if using heat-set inserts instead).

## Consumables / adhesives

| Item | Used for |
|------|----------|
| Hot glue (or double-sided foam tape / VHB) | Seating the servo body in the base bracket slot; gluing the plate on (if not screwed) |
| CA glue / epoxy (optional) | Securing the guide rods in the base bracket if the press-fit is loose |
| Thread-locker or nail polish (optional) | Keeping the pivot joints' retaining nuts from vibrating loose |

## Already-built / not sourced here

- **Resonance plate** — the existing physical part from the MFT26 "E.
  Modular" build (`hardware/docs/MFT26_devlog.pdf`), not something this
  BOM produces. See `plate_mount_diagram.png` for how it attaches.

## Per-build subset

- **`Plato_001_fsr_led`**: XIAO + FSR402 + 1kΩ + 330Ω + LED only.
- **`Plato_002_fsr_led_buzzer_vibration`**: adds the buzzer + vibration
  motor to `Plato_001`.
- **`Plato_003_fsr_led_buzzer`**: `Plato_002` minus the vibration motor.
- **`Plato_004_fsr_led_buzzer_vibration_lift`**: everything above (this
  file's full list).
- **`Plato_005_tact_lift`**: standalone — tact switch + servo + the same
  3D-printed mechanism only (drop the FSR/LED/buzzer/vibration row).
