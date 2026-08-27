# PLATO — Seeed XIAO ESP32C3 Prototype

Arduino firmware prototype for the Seeed Studio XIAO ESP32C3: reads a BME280
sensor, exposes it and a set of actuators over Wi-Fi (REST) and BLE (GATT),
and drives a status LED, a hobby servo, and a brushed DC motor.

## Hardware

| Function              | XIAO pin | ESP32C3 GPIO | Notes |
|-----------------------|----------|---------------|-------|
| I2C SDA (BME280)      | D4       | GPIO6         | Default `Wire` pins on this board |
| I2C SCL (BME280)      | D5       | GPIO7         | |
| Status LED            | D0       | GPIO2         | External LED + series resistor to GND. GPIO2 is a strapping pin — keep the load light |
| Servo signal          | D2       | GPIO4         | Servo needs its own 5V supply for anything beyond a micro servo; share GND with the board |
| Motor driver IN1      | D1       | GPIO3         | To a 2-pin H-bridge (DRV8833/TB6612-style) |
| Motor driver IN2      | D3       | GPIO5         | |

Pins are only used in `config.h` — change them there if your wiring differs.
GPIO8/GPIO9 (D8/D9, the BOOT strap pin) are intentionally left unused for
actuators.

## Firmware architecture

```
firmware/PLATO_XIAO_C3/
├── PLATO_XIAO_C3.ino   # setup/loop, shared JSON control protocol
├── config.h            # pins, Wi-Fi credentials, BLE UUIDs
├── SensorManager.*      # BME280 read (temperature/humidity/pressure)
├── ActuatorManager.*    # LED, servo, DC motor
├── NetManager.*         # Wi-Fi STA + REST API (built-in WebServer)
├── BleManager.*         # BLE GATT service (NimBLE-Arduino)
└── sketch.yaml          # arduino-cli profile (board + pinned library versions)
```

Both transports speak the same control JSON:
`{"led": true, "servo": 90, "motor": 150}` (any subset of fields).

- **HTTP**: `GET /api/status` returns sensor + actuator state;
  `POST /api/control` with a JSON body applies it.
- **BLE**: service `BLE_SERVICE_UUID` (see `config.h`) with a `NOTIFY` sensor
  characteristic and a `WRITE` control characteristic taking the same JSON.

ESP32C3 has a single 2.4GHz radio shared (time-multiplexed) between Wi-Fi and
BLE, so both work concurrently but under load you may see extra latency on
one side — fine for periodic sensor polling like this prototype.

## Setup

Install [arduino-cli](https://arduino.github.io/arduino-cli/), then from the
repo root:

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls \
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

`sketch.yaml` pins the exact board/library versions used here; with
arduino-cli >= 0.35 the `--profile` flag installs and uses them automatically
via `scripts/build.sh` / `scripts/upload.sh`. On an older arduino-cli, install
the libraries manually instead:

```bash
arduino-cli lib install "ArduinoJson" "Adafruit BME280 Library" \
  "ESP32Servo" "NimBLE-Arduino"
```

Edit `firmware/PLATO_XIAO_C3/config.h` and set `WIFI_SSID` / `WIFI_PASSWORD`
before flashing.

## Build & flash

```bash
scripts/build.sh
scripts/upload.sh /dev/ttyACM0   # adjust to your board's serial port
scripts/monitor.sh /dev/ttyACM0
```

(On Windows/WSL or macOS the port will look like `COM3` or
`/dev/cu.usbmodem*`.)

## Testing

**HTTP** (once connected to Wi-Fi; IP is printed on the serial monitor):

```bash
curl http://<device-ip>/api/status
curl -X POST http://<device-ip>/api/control -d '{"led":true,"servo":45,"motor":-120}'
```

**BLE**: scan for `PLATO-XIAO-C3` with nRF Connect (or similar), subscribe to
the sensor characteristic for notifications, and write JSON to the control
characteristic.

## Extending

- Swap the BME280 for another I2C/analog sensor by editing `SensorManager`.
- Add more actuators by extending `ActuatorManager` and the shared control
  JSON schema in `PLATO_XIAO_C3.ino`.
- For an XIAO ESP32S3 build, add a second `sketch.yaml` profile with
  `fqbn: esp32:esp32:XIAO_ESP32S3` — the application code is board-agnostic
  as long as the pins in `config.h` are valid on that board.
