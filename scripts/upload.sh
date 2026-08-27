#!/usr/bin/env bash
# Compile and flash the PLATO firmware to a XIAO ESP32C3.
# Usage: scripts/upload.sh [port]   (default: /dev/ttyACM0)
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../firmware/PLATO_XIAO_C3" && pwd)"

arduino-cli compile --profile xiao_c3 "$SKETCH_DIR"
arduino-cli upload --profile xiao_c3 -p "$PORT" "$SKETCH_DIR"
