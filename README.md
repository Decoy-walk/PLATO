# PLATO — Embodied Memory via Material Interaction

Research prototype exploring how sensed hand↔object interaction (grip force
and physiological/bio signal) with a 3D structure can be logged and analyzed
to study **embodied memory**. A sensing node is embedded in the 3D structure,
powered wirelessly (no battery/cable through the object), and streams
interaction data over BLE to a PC bridge that logs it to CSV, git, and/or a
Google Sheet.

```
 hand  ⇄  3D structure (FSR402 + NJL5513R + BPW34, wirelessly powered)
                 │  BLE notify (~10 Hz JSON)
                 ▼
         bridge/plato_bridge.py  (PC)
                 │
        ┌────────┼─────────┐
        ▼        ▼         ▼
   data/*.csv   git commit   Google Sheets
```

## Hardware

| Component                          | Role                                                              |
|-------------------------------------|---------------------------------------------------------------------|
| Seeed XIAO ESP32C3                  | MCU: samples sensors, runs the BLE peripheral                       |
| FSR402 (short tail)                 | Grip/press force where the hand contacts the structure               |
| NJL5513R                            | Reflectance photo-IC + LED — bio/pulse (PPG) signal through the skin |
| BPW34                               | Bare PIN photodiode — ambient-light reference to cancel ambient noise from the PPG channel |
| Wireless power transfer (Tx/Rx pair)| Powers the embedded node with no wire passing through the structure  |

### Pins (Seeed XIAO ESP32C3 silkscreen labels)

| Function                | XIAO pin | ESP32C3 GPIO   | Notes |
|--------------------------|----------|-----------------|-------|
| FSR402 (voltage divider) | D0       | GPIO2 (ADC1_CH2)| `3V3 — FSR402 — D0 — 10kΩ — GND` |
| NJL5513R output          | D1       | GPIO3 (ADC1_CH3)| Photo-IC output goes straight to the ADC (internal amp); add a 100 nF cap output→GND and 0.1 µF supply decoupling per its datasheet |
| BPW34 (ambient ref.)     | D2       | GPIO4 (ADC1_CH4)| Reverse-biased: cathode→3V3, anode→D2, D2→1 MΩ→GND. Mount away from the NJL5513R's LED so it only sees ambient light |
| PPG reflectance LED      | D3       | GPIO5           | Green/IR LED + current-limit resistor, aimed at the same skin contact point as NJL5513R |
| Grip feedback LED        | D4       | GPIO6           | Lights while `fsr > FSR_GRIP_THRESHOLD` — real-time affordance for the person interacting with the structure |

All three analog sensors are wired to ADC1-only pins on purpose: ESP32-C3's
ADC2 conflicts with the radio, and this node is broadcasting over BLE
continuously.

> The FSR402/NJL5513R/BPW34 bias circuits above are the minimum viable
> analog front end for a prototype. For research-grade PPG/HRV, a dedicated
> transimpedance-amp + bandpass front end (e.g. MAX30101-class AFE) will be
> materially cleaner than reading a bare photodiode through the MCU's ADC —
> treat this build as "good enough to detect interaction, not clinical
> signal quality."

## Firmware (`firmware/PLATO_XIAO_C3/`)

```
PLATO_XIAO_C3.ino     # setup/loop
config.h               # pins, BLE UUIDs, sampling + detection constants
BioSensorManager.*     # FSR grip force, NJL5513R/BPW34 ambient-cancelled PPG
BleManager.*           # BLE GATT service (NimBLE-Arduino)
sketch.yaml            # arduino-cli profile (board + pinned library version)
```

Every `BLE_NOTIFY_INTERVAL_MS` (10 Hz default) the node notifies a JSON
payload on `BLE_SENSOR_CHAR_UUID`:

```json
{"fsr": 1830, "grip": true, "ambient": 512, "bpm": 74}
```

- `fsr` / `grip`: raw ADC reading and threshold-crossing boolean for press/grip force.
- `ambient`: raw BPW34 reading (also useful on its own as a lighting-condition log).
- `bpm`: instantaneous heart rate from the ambient-cancelled PPG beat detector (0 until a beat has been seen).

The PPG algorithm (`BioSensorManager::sample`) is a simple dual-EMA
threshold detector, not a validated HRV pipeline — see `config.h` for the
tunable constants (`PPG_*`) if beats aren't triggering reliably against your
skin tone/contact pressure.

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

Since the node is wirelessly powered and BLE-only (no on-board Wi-Fi
credentials/HTTP stack to keep the power budget and complexity down), the PC
running the bridge is what has real internet access — it fans each reading
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
  device whose name starts with this, so adding more structures/nodes later
  is just flashing each with a distinct `BLE_DEVICE_NAME` in `config.h`
  (e.g. `PLATO-A`, `PLATO-B`) — no bridge code changes needed.
- `--csv` (default `data/plato_log.csv`): local log, one row per reading,
  columns `timestamp_utc,node,fsr,grip,ambient,bpm`.
- `--git-commit-every N`: commit+push the CSV every N new rows (0 disables).
  Requires the bridge to run somewhere with push access to this repo.
- `--sheets-url` / `GOOGLE_SHEETS_WEBHOOK_URL`: POSTs each row as JSON to a
  Google Apps Script Web App. See `bridge/google_apps_script.gs` for the
  ~10-line script to paste into a target Sheet's Apps Script editor and
  deploy as a Web App (Extensions → Apps Script → Deploy → New deployment).

If a node disconnects mid-session, restart the bridge — it's a straight
scan-and-stream loop, no reconnect logic yet.

## Extending

- Multiple structures/nodes: give each a unique `BLE_DEVICE_NAME`; the
  bridge already fans in every matching device concurrently.
- Swap/add sensors by extending `BioSensorManager` and the JSON schema in
  `PLATO_XIAO_C3.ino` — keep new analog channels on ADC1 pins (GPIO0-4 on
  this chip).
- For an XIAO ESP32S3 build, add a second `sketch.yaml` profile with
  `fqbn: esp32:esp32:XIAO_ESP32S3` and re-check its ADC1 pin map before
  reusing the same `config.h` pin assignments.
