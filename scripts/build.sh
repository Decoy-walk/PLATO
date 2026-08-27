#!/usr/bin/env bash
# Compile the PLATO firmware for the Seeed XIAO ESP32C3 using arduino-cli.
set -euo pipefail

SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../firmware/PLATO_XIAO_C3" && pwd)"

arduino-cli compile --profile xiao_c3 "$SKETCH_DIR"
