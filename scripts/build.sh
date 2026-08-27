#!/usr/bin/env bash
# Compile the PLATO research firmware for the Seeed XIAO SAMD21 using arduino-cli.
set -euo pipefail

SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../firmware/PLATO_XIAO_SAMD21" && pwd)"

arduino-cli compile --profile xiao_samd21 "$SKETCH_DIR"
