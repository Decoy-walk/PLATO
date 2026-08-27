#!/usr/bin/env python3
"""Serial bridge for PLATO folding-block sensing nodes.

Reads the wired-USB fold-state stream from one or more XIAO SAMD21 PLATO
folding-block nodes (see firmware/PLATO_XIAO_SAMD21 - SAMD21 has no
on-board radio, so this is a wired interim transport pending a BLE-module
decision), logs it to a local CSV, and optionally git-commits the log
and/or forwards each row to a Google Sheet via an Apps Script Web App.
"""
from __future__ import annotations

import argparse
import csv
import os
import subprocess
import sys
import threading
from datetime import datetime, timezone
from pathlib import Path

import requests
import serial

# Must match SERIAL_SYNC_BYTE_1/2 and the payload layout in
# firmware/PLATO_XIAO_SAMD21/config.h and SerialTransport.cpp.
SYNC_BYTE_1 = 0xAA
SYNC_BYTE_2 = 0x55
PAYLOAD_LEN = 4

HINGE_COUNT = 20
CSV_FIELDS = ["timestamp_utc", "node", "haptic_active", "haptic_enabled"] + [
    f"hinge_{i:02d}" for i in range(HINGE_COUNT)
]


def decode_payload(data: bytes) -> dict:
    """Unpacks the firmware's 4-byte frame body: 3 bytes of 20-hinge fold
    bitmask (LSB first) + 1 flags byte (bit0 haptic active, bit1 haptic
    feedback condition enabled)."""
    if len(data) != PAYLOAD_LEN:
        raise ValueError(f"expected {PAYLOAD_LEN} bytes, got {len(data)}")
    bitmask = data[0] | (data[1] << 8) | (data[2] << 16)
    flags = data[3]

    reading = {
        "haptic_active": bool(flags & 0x01),
        "haptic_enabled": bool(flags & 0x02),
    }
    for i in range(HINGE_COUNT):
        reading[f"hinge_{i:02d}"] = int(bool(bitmask & (1 << i)))
    return reading


class Sink:
    """Fans out each reading to CSV, git, and/or Google Sheets."""

    def __init__(self, csv_path: Path, git_commit_every: int, sheets_url: str | None):
        self.csv_path = csv_path
        self.git_commit_every = git_commit_every
        self.sheets_url = sheets_url
        self._rows_since_commit = 0
        self._ensure_csv_header()

    def _ensure_csv_header(self) -> None:
        self.csv_path.parent.mkdir(parents=True, exist_ok=True)
        if not self.csv_path.exists():
            with self.csv_path.open("w", newline="") as f:
                csv.DictWriter(f, fieldnames=CSV_FIELDS).writeheader()

    def record(self, node: str, reading: dict) -> None:
        row = {"timestamp_utc": datetime.now(timezone.utc).isoformat(), "node": node}
        row.update(reading)

        with self.csv_path.open("a", newline="") as f:
            csv.DictWriter(f, fieldnames=CSV_FIELDS).writerow(row)

        if self.sheets_url:
            self._post_to_sheets(row)

        if self.git_commit_every:
            self._rows_since_commit += 1
            if self._rows_since_commit >= self.git_commit_every:
                self._rows_since_commit = 0
                self._git_commit()

    def _post_to_sheets(self, row: dict) -> None:
        try:
            requests.post(self.sheets_url, json=row, timeout=5)
        except requests.RequestException as exc:
            print(f"[sheets] post failed: {exc}", file=sys.stderr)

    def _git_commit(self) -> None:
        repo_dir = self.csv_path.resolve().parent
        try:
            subprocess.run(["git", "add", str(self.csv_path)], cwd=repo_dir, check=True)
            subprocess.run(
                ["git", "commit", "-m", f"Log PLATO sensor data ({datetime.now():%Y-%m-%d %H:%M})"],
                cwd=repo_dir,
                check=True,
            )
            subprocess.run(["git", "push"], cwd=repo_dir, check=True)
        except subprocess.CalledProcessError as exc:
            print(f"[git] commit/push failed: {exc}", file=sys.stderr)


def read_frames(ser: serial.Serial):
    """Generator that yields each frame's 4-byte payload, resyncing on the
    2-byte marker before every frame (cheap enough at ~20 Hz, and self-heals
    if a byte is ever lost/corrupted on the wire)."""
    while True:
        b = ser.read(1)
        if not b or b[0] != SYNC_BYTE_1:
            continue
        b2 = ser.read(1)
        if not b2 or b2[0] != SYNC_BYTE_2:
            continue
        payload = ser.read(PAYLOAD_LEN)
        if len(payload) != PAYLOAD_LEN:
            continue
        yield payload


def stream_node(port: str, baud: int, sink: Sink) -> None:
    node_name = port
    with serial.Serial(port, baud, timeout=1) as ser:
        print(f"[{node_name}] connected, streaming...")
        for payload in read_frames(ser):
            try:
                reading = decode_payload(payload)
            except ValueError as exc:
                print(f"[{node_name}] bad payload: {exc}", file=sys.stderr)
                continue
            sink.record(node_name, reading)
            folded = [i for i in range(HINGE_COUNT) if reading[f"hinge_{i:02d}"]]
            print(f"[{node_name}] folded={folded} haptic_active={reading['haptic_active']}")


def run(args: argparse.Namespace) -> int:
    sink = Sink(Path(args.csv), args.git_commit_every, args.sheets_url)

    ports = [p.strip() for p in args.ports.split(",") if p.strip()]
    if not ports:
        print("No serial ports given.", file=sys.stderr)
        return 1

    threads = [
        threading.Thread(target=stream_node, args=(port, args.baud, sink), daemon=True)
        for port in ports
    ]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Serial bridge: PLATO folding-block node(s) -> CSV / git / Google Sheets"
    )
    parser.add_argument(
        "--ports",
        default="/dev/ttyACM0",
        help="comma-separated serial port(s), one per node (default: %(default)s)",
    )
    parser.add_argument("--baud", type=int, default=115200, help="must match SERIAL_BAUD in config.h")
    parser.add_argument(
        "--csv", default="data/plato_log.csv", help="local CSV log path (default: %(default)s)"
    )
    parser.add_argument(
        "--git-commit-every",
        type=int,
        default=0,
        help="commit+push the CSV every N new rows (0 = disabled)",
    )
    parser.add_argument(
        "--sheets-url",
        default=os.environ.get("GOOGLE_SHEETS_WEBHOOK_URL"),
        help="Google Apps Script Web App URL (or set GOOGLE_SHEETS_WEBHOOK_URL)",
    )
    return parser.parse_args()


if __name__ == "__main__":
    raise SystemExit(run(parse_args()))
