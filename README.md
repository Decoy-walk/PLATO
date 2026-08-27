# PLATO — Embodied Memory via Material Interaction

Research prototype for testing whether tactile-motor interaction with a
physical folding-block device activates procedural memory as a
supplementary channel, improving recall vs. text-only encoding. A sensing
node embedded in the folding block tracks all 20 hinges and streams the
fold state over BLE to a PC bridge that logs it to CSV, git, and/or a
Google Sheet.

```
 hand  ⇄  folding block (20 flex-sensor hinges + haptic actuator)
                 │  BLE notify (~20 Hz, 4-byte fold bitmask)
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
| Seeed XIAO ESP32C3               | MCU: scans the 20 hinges, drives haptic feedback, runs BLE |
| 20× flex sensor (one per hinge)  | Captures the fold sequence as the hand manipulates the block |
| 2× CD74HC4067 (16-channel analog mux) | Multiplexes 20 flex-sensor channels onto 1 ADC pin      |
| Haptic/ERM motor + driver transistor | "Block + haptic feedback" condition's feedback channel |

### Why a mux instead of more ADC pins or an I2C ADC expander

XIAO ESP32C3 only exposes 3-4 ADC1-capable pins (ADC2 conflicts with the
BLE radio), nowhere near 20. Rather than adding 5× I2C ADC boards
(e.g. ADS1115) to get 20 native channels, this design keeps the channel
count matched to what the board can actually do: two CD74HC4067 16-channel
analog muxes (32 channels available, 20 used) share 4 address lines and
time-multiplex all 20 flex sensors onto a **single** ADC pin. Only one
mux's active-low `EN` is driven low at a time; the other's output stays
high-impedance, so both chips' common `SIG` pins can tie to the same ADC
input. Net cost: 1 ADC pin + 6 digital GPIO instead of 20 ADC pins or extra
I2C ADC hardware.

### Pins (Seeed XIAO ESP32C3 silkscreen labels)

| Function                     | XIAO pin | ESP32C3 GPIO | Notes |
|-------------------------------|----------|--------------|-------|
| Mux common analog output      | D0       | GPIO2 (ADC1_CH2) | Both CD74HC4067 `SIG` pins tie here |
| Mux address S0                | D1       | GPIO3        | Shared between both mux chips |
| Mux address S1                | D2       | GPIO4        | |
| Mux address S2                | D3       | GPIO5        | |
| Mux address S3                | D4       | GPIO6        | |
| Mux A enable (active-low)     | D5       | GPIO7        | Selects hinges 0-15 |
| Mux B enable (active-low)     | D6       | GPIO21       | Selects hinges 16-19 (channels 4-15 of mux B unused) |
| Haptic actuator PWM           | D7       | GPIO20       | To a transistor driving the ERM/LRA motor (+ flyback diode) |
| Status LED                    | D10      | GPIO10       | Lit while any hinge reads folded |

Each flex sensor forms its own voltage divider into its mux channel:
`3V3 — flexSensor — muxChannel — 10kΩ — GND`. Both mux chips' `S0-S3` are
wired in parallel; `EN` pins are separate so the firmware can choose which
16-channel bank is currently driving the shared `SIG`/ADC line.

## Two separate firmware builds

- `firmware/PLATO_XIAO_C3/` — the research build described below (ESP32C3,
  20-hinge sensing, BLE, PC bridge).
- `firmware/PLATO_EXHIBIT_SAMD21/` — a standalone exhibition-only build
  (Seeeduino XIAO/SAMD21, single flex sensor + LED, no wireless). See its
  own README for wiring and setup. Built for a specific 7-day exhibition
  deadline, not the formal study.

## Firmware (`firmware/PLATO_XIAO_C3/`)

```
PLATO_XIAO_C3.ino    # setup/loop, serial commands, condition toggling
config.h              # pins, mux/timing/haptic constants, BLE UUIDs
FlexMuxManager.*      # mux channel scanning, per-hinge calibration (NVS), fold bitmask
HapticManager.*       # non-blocking haptic pulse control
BleManager.*          # BLE GATT service (NimBLE-Arduino)
sketch.yaml           # arduino-cli profile (board + pinned library version)
```

### Calibration (per-hinge variance)

Flex-sensor voltage-divider output varies hinge-to-hinge (resistance range,
mounting preload). Rather than a single global threshold, each hinge gets
its own calibrated min/max:

- Send `c` over the serial monitor to start a 15 s calibration window
  (`CALIBRATION_DURATION_MS`); fold/unfold every hinge through its full
  range while it runs.
- Results are saved to NVS (`Preferences`, namespace `platocal`) and printed
  per-hinge, so you have a `min/max/range` table to report alongside the
  data — this is the explicit bounding of per-hinge variance flagged as an
  internal-validity concern for the write-up.
- Run this once per session (or per participant) rather than relying on a
  factory-flashed default, since divider output can drift with sensor wear.

### Condition toggling

`h` over serial toggles haptic feedback on/off at runtime, so the same
flashed firmware serves both the **Block (motor only)** and **Block +
haptic feedback** study arms without reflashing between participants. The
condition flag is included in every BLE notification (see below) so it's
logged automatically rather than needing to be tracked by hand.

### BLE payload

Every `BLE_NOTIFY_INTERVAL_MS` (20 Hz default) the node notifies a 4-byte
binary payload on `BLE_SENSOR_CHAR_UUID` — binary rather than JSON so a
20-hinge reading always fits one ATT notification regardless of whether
the central negotiated a larger MTU:

| Byte | Content |
|------|---------|
| 0-2  | 20-bit fold bitmask, LSB first (bit *i* = hinge *i* folded) |
| 3    | bit0: haptic pulse currently firing · bit1: haptic-feedback condition enabled |

### Setup

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls \
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

`sketch.yaml` pins the board/library versions; with arduino-cli >= 0.35 the
`--profile` flag (used by the scripts below) installs and uses them
automatically. On an older arduino-cli, install manually instead:

```bash
arduino-cli lib install "NimBLE-Arduino"
```

### Build & flash

```bash
scripts/build.sh
scripts/upload.sh /dev/ttyACM0   # adjust to your board's serial port
scripts/monitor.sh /dev/ttyACM0
```

## PC bridge (`bridge/`)

The node is BLE-only (no on-board Wi-Fi) to keep the embedded side simple;
the PC running the bridge has real internet access and fans each reading
out to a local CSV, an optional git commit+push, and/or a Google Sheet.

```bash
pip install -r bridge/requirements.txt

# CSV only:
python bridge/plato_bridge.py

# CSV + auto-commit every 50 rows + push to Google Sheets:
python bridge/plato_bridge.py \
  --git-commit-every 50 \
  --sheets-url "https://script.google.com/macros/s/XXXX/exec"
```

- `--device-prefix` (default `PLATO-`): connects to every advertised BLE
  device whose name starts with this, so multiple structures/nodes are just
  a matter of flashing each with a distinct `BLE_DEVICE_NAME` — no bridge
  code changes needed.
- `--csv` (default `data/plato_log.csv`): one row per BLE notification,
  columns `timestamp_utc,node,haptic_active,haptic_enabled,hinge_00..hinge_19`.
  Logging every snapshot (not just transitions) means a dropped BLE
  notification never loses an event — fold-sequence order and per-hinge
  transition timing (for the Kendall's Tau / Spearman's rho / edit-distance
  analyses) are reconstructed from the time series afterward, not relied on
  from the stream itself.
- `--git-commit-every N`: commit+push the CSV every N new rows (0 disables).
  Requires the bridge to run somewhere with push access to this repo.
- `--sheets-url` / `GOOGLE_SHEETS_WEBHOOK_URL`: POSTs each row as JSON to a
  Google Apps Script Web App. See `bridge/google_apps_script.gs` for the
  script to paste into a target Sheet's Apps Script editor and deploy as a
  Web App (Extensions → Apps Script → Deploy → New deployment).

If a node disconnects mid-session, restart the bridge — it's a straight
scan-and-stream loop, no reconnect logic yet.

## Extending

- Multiple structures/nodes: give each a unique `BLE_DEVICE_NAME`; the
  bridge already fans in every matching device concurrently.
- More/fewer hinges: `FlexMuxManager::kChannelCount` and the mux wiring
  scale to any count up to 32 (two 16-channel muxes) without firmware
  changes beyond that constant.
- For an XIAO ESP32S3 build, add a second `sketch.yaml` profile with
  `fqbn: esp32:esp32:XIAO_ESP32S3` — the mux approach is unaffected since it
  only ever needs one ADC-capable pin.
