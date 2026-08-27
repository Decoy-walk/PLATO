# PLATO — Embodied Memory via Material Interaction

Research prototype for testing whether tactile-motor interaction with a
physical folding-block device activates procedural memory as a
supplementary channel, improving recall vs. text-only encoding. A sensing
node embedded in the folding block tracks all 20 hinges and streams the
fold state over wired USB serial to a PC bridge that logs it to CSV, git,
and/or a Google Sheet.

```
 hand  ⇄  folding block (20 flex-sensor hinges + haptic actuator)
                 │  USB serial (~20 Hz, framed 4-byte fold bitmask)
                 ▼
         bridge/plato_bridge.py  (PC)
                 │
        ┌────────┼─────────┐
        ▼        ▼         ▼
   data/*.csv   git commit   Google Sheets
```

## Hardware

| Component                       | Role                                                    |
|-----------------------------------|------------------------------------------------------------|
| Seeed XIAO SAMD21                | MCU: scans the 20 hinges, drives haptic feedback, streams over USB |
| 20× flex sensor (one per hinge)  | Captures the fold sequence as the hand manipulates the block |
| 2× CD74HC4067 (16-channel analog mux) | Multiplexes 20 flex-sensor channels onto 1 ADC pin      |
| Haptic/ERM motor + driver transistor | "Block + haptic feedback" condition's feedback channel |

The board was originally ESP32C3 (see git history) and moved permanently to
SAMD21. SAMD21 has no on-board radio, so wireless logging is an open
question — see "Wireless is still undecided" below — and this build uses a
wired USB serial connection as the interim transport.

### Why a mux instead of more ADC pins or an I2C ADC expander

Neither ESP32C3 nor SAMD21's XIAO form factor exposes 20 native ADC pins.
Rather than adding 5× I2C ADC boards (e.g. ADS1115), this design keeps the
channel count matched to what the board can actually do: two CD74HC4067
16-channel analog muxes (32 channels available, 20 used) share 4 address
lines and time-multiplex all 20 flex sensors onto a **single** ADC pin.
Only one mux's active-low `EN` is driven low at a time; the other's output
stays high-impedance, so both chips' common `SIG` pins can tie to the same
ADC input. Net cost: 1 ADC pin + 6 digital GPIO instead of 20 ADC pins or
extra I2C ADC hardware. (SAMD21 actually has no ADC-vs-radio pin
restriction the way ESP32C3's ADC2 did, but the mux wiring is kept
identical for continuity with the physical harness.)

### Pins (Seeed XIAO SAMD21 silkscreen labels)

| Function                     | XIAO pin | Notes |
|-------------------------------|----------|-------|
| Mux common analog output      | D0 (A0)  | Both CD74HC4067 `SIG` pins tie here |
| Mux address S0                | D1       | Shared between both mux chips |
| Mux address S1                | D2       | |
| Mux address S2                | D3       | |
| Mux address S3                | D4       | |
| Mux A enable (active-low)     | D5       | Selects hinges 0-15 |
| Mux B enable (active-low)     | D6       | Selects hinges 16-19 (channels 4-15 of mux B unused) |
| Haptic actuator PWM           | D7       | To a transistor driving the ERM/LRA motor (+ flyback diode) |
| Status LED                    | D10      | Lit while any hinge reads folded |

Each flex sensor forms its own voltage divider into its mux channel:
`3V3 — flexSensor — muxChannel — 10kΩ — GND`. Both mux chips' `S0-S3` are
wired in parallel; `EN` pins are separate so the firmware can choose which
16-channel bank is currently driving the shared `SIG`/ADC line.

## Two separate firmware builds

- `firmware/PLATO_XIAO_SAMD21/` — the research build described below
  (SAMD21, 20-hinge sensing, wired serial, PC bridge).
- `firmware/PLATO_EXHIBIT_SAMD21/` — a standalone exhibition-only build
  (also SAMD21, but single flex sensor + LED, no per-hinge sensing, no
  logging). See its own README for wiring and setup. Built for a specific
  7-day exhibition deadline, not the formal study — keep it separate rather
  than merging the two, since they serve different purposes and the
  exhibit piece is a different physical object (single open/close DOF, not
  20 independent hinges).

## Firmware (`firmware/PLATO_XIAO_SAMD21/`)

```
PLATO_XIAO_SAMD21.ino  # setup/loop, serial commands, condition toggling
config.h                # pins, mux/timing/haptic constants, serial framing
FlexMuxManager.*        # mux channel scanning, per-hinge calibration (RAM-only), fold bitmask
HapticManager.*         # non-blocking haptic pulse control
SerialTransport.*       # wired USB serial framing (interim - see below)
sketch.yaml             # arduino-cli profile (board + platform version)
```

### Wireless is still undecided

SAMD21 has no BLE/Wi-Fi radio. Two options were on the table:

1. **Wired USB serial** (what's implemented now) — zero extra hardware,
   most reliable, but reintroduces the "does a cable undermine the
   naturalistic-interaction claim" concern raised for the write-up.
2. **External BLE UART module** (e.g. a DA14531/nRF52-based UART-BLE
   bridge) — keeps the wireless/naturalistic-use framing intact at the
   cost of extra per-node hardware and wiring.

`SerialTransport` is deliberately the only place that knows about the wire
protocol — `PLATO_XIAO_SAMD21.ino` just calls `transport.send(payload, len)`.
Swapping in a `BleUartTransport` later (same `send()` signature, framed
over a BLE UART module's serial passthrough instead of the native USB port)
should be a small, isolated change, not a rewrite.

### Calibration (per-hinge variance)

Flex-sensor voltage-divider output varies hinge-to-hinge (resistance range,
mounting preload). Rather than a single global threshold, each hinge gets
its own calibrated min/max:

- Send `c` over the serial monitor to start a 15 s calibration window
  (`CALIBRATION_DURATION_MS`); fold/unfold every hinge through its full
  range while it runs.
- Calibration is **RAM-only** (SAMD21 has no direct equivalent to ESP32's
  `Preferences`/NVS without another library dependency) and is printed
  per-hinge, so you have a `min/max/range` table to report alongside the
  data — the explicit bounding of per-hinge variance flagged as an
  internal-validity concern for the write-up. Because it's RAM-only, **run
  `c` again after every power cycle** — this matches the documented
  per-session calibration protocol anyway (run once per session/participant
  rather than trusting a stale factory default).

### Condition toggling

`h` over serial toggles haptic feedback on/off at runtime, so the same
flashed firmware serves both the **Block (motor only)** and **Block +
haptic feedback** study arms without reflashing between participants. The
condition flag is included in every serial frame (see below) so it's
logged automatically rather than needing to be tracked by hand.

### Serial frame format

Every `NOTIFY_INTERVAL_MS` (20 Hz default) the node writes a 6-byte frame
to the USB serial port: 2 marker bytes (so the PC-side reader can
byte-align on this continuous stream even if it connects mid-frame, and
self-heal from an occasional dropped/corrupted byte) followed by the same
4-byte payload the ESP32C3 build used to send over BLE:

| Byte | Content |
|------|---------|
| 0    | `0xAA` sync byte 1 |
| 1    | `0x55` sync byte 2 |
| 2-4  | 20-bit fold bitmask, LSB first (bit *i* = hinge *i* folded) |
| 5    | bit0: haptic pulse currently firing · bit1: haptic-feedback condition enabled |

### Setup

```bash
arduino-cli config add board_manager.additional_urls \
  https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
arduino-cli core update-index
arduino-cli core install Seeeduino:samd
```

`sketch.yaml` pins the board/platform version; with arduino-cli >= 0.35 the
`--profile` flag (used by the scripts below) installs and uses it
automatically. No external libraries are needed (Arduino SAMD core only).

In Arduino IDE: Tools > Board > **Seeed SAMD Boards > Seeeduino XIAO**.

### Build & flash

```bash
scripts/build.sh
scripts/upload.sh /dev/ttyACM0   # adjust to your board's serial port
scripts/monitor.sh /dev/ttyACM0
```

## PC bridge (`bridge/`)

The node streams over wired USB serial (see "Wireless is still undecided"
above); the bridge opens that port directly and fans each reading out to a
local CSV, an optional git commit+push, and/or a Google Sheet.

```bash
pip install -r bridge/requirements.txt

# CSV only:
python bridge/plato_bridge.py --ports /dev/ttyACM0

# CSV + auto-commit every 50 rows + push to Google Sheets:
python bridge/plato_bridge.py \
  --ports /dev/ttyACM0 \
  --git-commit-every 50 \
  --sheets-url "https://script.google.com/macros/s/XXXX/exec"
```

- `--ports` (default `/dev/ttyACM0`): comma-separated serial port(s), one
  per physical node — each runs in its own thread, so multiple structures
  just means multiple `--ports` entries (e.g. `/dev/ttyACM0,/dev/ttyACM1`).
- `--baud` (default 115200): must match `SERIAL_BAUD` in `config.h`.
- `--csv` (default `data/plato_log.csv`): one row per frame, columns
  `timestamp_utc,node,haptic_active,haptic_enabled,hinge_00..hinge_19`.
  Logging every snapshot (not just transitions) means one corrupted/lost
  frame never loses an event — fold-sequence order and per-hinge transition
  timing (for the Kendall's Tau / Spearman's rho / edit-distance analyses)
  are reconstructed from the time series afterward, not relied on from the
  stream itself.
- `--git-commit-every N`: commit+push the CSV every N new rows (0 disables).
  Requires the bridge to run somewhere with push access to this repo.
- `--sheets-url` / `GOOGLE_SHEETS_WEBHOOK_URL`: POSTs each row as JSON to a
  Google Apps Script Web App. See `bridge/google_apps_script.gs` for the
  script to paste into a target Sheet's Apps Script editor and deploy as a
  Web App (Extensions → Apps Script → Deploy → New deployment).

If a node disconnects mid-session, restart the bridge — it's a straight
open-and-stream loop per port, no reconnect logic yet.

## Extending

- Multiple structures/nodes: connect each over its own USB port and add it
  to `--ports`; the bridge already fans in every port concurrently.
- More/fewer hinges: `FlexMuxManager::kChannelCount` and the mux wiring
  scale to any count up to 32 (two 16-channel muxes) without firmware
  changes beyond that constant. Keep any CAD change to hinge count in sync
  with this constant (see `hardware/cad/README.md`).
- Adding wireless: implement a `BleUartTransport` with the same
  `send(data, len)` signature as `SerialTransport` and swap it in
  `PLATO_XIAO_SAMD21.ino` — the rest of the firmware doesn't need to change.
