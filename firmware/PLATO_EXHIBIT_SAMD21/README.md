# Exhibition build (Seeeduino XIAO, SAMD21)

Standalone piece for the 7-day exhibition: no wireless, no PC bridge, no
per-hinge sensing. One flex sensor reads how open/closed the ball is; one
LED brightens as it opens. This is intentionally separate from
`firmware/PLATO_XIAO_C3/` (the ESP32C3 research build) — different board,
different purpose.

## Wiring

- Flex sensor: `3V3 — flex sensor — A0 — 10kΩ — GND`
- LED: `D1 — 330Ω resistor — LED anode`; LED cathode → `GND`

## Setup (one-time)

This board uses the **Seeeduino** SAMD package, not the `esp32` one from the
other firmware:

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

No external libraries — Arduino SAMD core only, so there's nothing to
install beyond the board package above.

## Tuning for the exhibit floor

All the knobs are constants at the top of the `.ino`:

- `IDLE_TIMEOUT_MS` / `IDLE_MOVEMENT_THRESHOLD`: how long/still before the
  idle "breathing" animation kicks in.
- `SMOOTH_ALPHA`: raise it for a snappier response, lower it if the LED
  flickers on sensor noise.
- Calibration is automatic and only ever widens its observed range, so it
  doesn't need resetting each morning — just make sure someone fully opens
  and fully closes the ball once after power-up so the range isn't too
  narrow for the first few visitors.
