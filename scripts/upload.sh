#!/usr/bin/env bash
# Compile and flash the PLATO research firmware to a XIAO SAMD21.
# Usage: scripts/upload.sh [port]   (default: /dev/ttyACM0)
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"
SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../firmware/PLATO_XIAO_SAMD21" && pwd)"

arduino-cli compile --profile xiao_samd21 "$SKETCH_DIR"
arduino-cli upload --profile xiao_samd21 -p "$PORT" "$SKETCH_DIR"
