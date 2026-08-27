#!/usr/bin/env bash
# Open a serial monitor to the XIAO ESP32C3.
# Usage: scripts/monitor.sh [port]   (default: /dev/ttyACM0)
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"

arduino-cli monitor -p "$PORT" -c baudrate=115200
