#!/usr/bin/env python3
"""BLE bridge for PLATO folding-block sensing nodes.

Connects to one or more XIAO ESP32C3 PLATO folding-block nodes (see
firmware/PLATO_XIAO_C3), logs their BLE fold-state notifications to a local
CSV, and optionally git-commits the log and/or forwards each row to a
Google Sheet via an Apps Script Web App.
"""
from __future__ import annotations

import argparse
import asyncio
import csv
import os
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

import requests
from bleak import BleakClient, BleakScanner

# Must match BLE_SENSOR_CHAR_UUID in firmware/PLATO_XIAO_C3/config.h
SENSOR_CHAR_UUID = "6f2a0002-8b1e-4a3e-9d0a-0000000000a1"

HINGE_COUNT = 20
CSV_FIELDS = ["timestamp_utc", "node", "haptic_active", "haptic_enabled"] + [
    f"hinge_{i:02d}" for i in range(HINGE_COUNT)
]


def decode_payload(data: bytes) -> dict:
    """Unpacks the firmware's 4-byte notification: 3 bytes of 20-hinge fold
    bitmask (LSB first) + 1 flags byte (bit0 haptic active, bit1 haptic
    feedback condition enabled)."""
    if len(data) < 4:
        raise ValueError(f"expected 4 bytes, got {len(data)}")
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


def make_notify_handler(node_name: str, sink: Sink):
    def _handler(_sender, data: bytearray) -> None:
        try:
            reading = decode_payload(bytes(data))
        except ValueError as exc:
            print(f"[{node_name}] bad payload: {exc}", file=sys.stderr)
            return
        sink.record(node_name, reading)
        folded = [i for i in range(HINGE_COUNT) if reading[f"hinge_{i:02d}"]]
        print(f"[{node_name}] folded={folded} haptic_active={reading['haptic_active']}")

    return _handler


async def stream_node(device, sink: Sink) -> None:
    node_name = device.name or device.address
    async with BleakClient(device) as client:
        await client.start_notify(SENSOR_CHAR_UUID, make_notify_handler(node_name, sink))
        print(f"[{node_name}] connected, streaming...")
        while client.is_connected:
            await asyncio.sleep(1)
    print(f"[{node_name}] disconnected")


async def run(args: argparse.Namespace) -> int:
    sink = Sink(Path(args.csv), args.git_commit_every, args.sheets_url)

    print(f"Scanning {args.scan_timeout}s for devices starting with '{args.device_prefix}'...")
    devices = await BleakScanner.discover(timeout=args.scan_timeout)
    targets = [d for d in devices if d.name and d.name.startswith(args.device_prefix)]

    if not targets:
        print("No matching PLATO nodes found.", file=sys.stderr)
        return 1

    print(f"Found {len(targets)} node(s): {[d.name for d in targets]}")
    await asyncio.gather(*(stream_node(d, sink) for d in targets))
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="BLE bridge: PLATO folding-block node(s) -> CSV / git / Google Sheets"
    )
    parser.add_argument(
        "--device-prefix",
        default="PLATO-",
        help="connect to any advertised BLE name starting with this (default: %(default)s)",
    )
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
    parser.add_argument("--scan-timeout", type=float, default=5.0)
    return parser.parse_args()


if __name__ == "__main__":
    raise SystemExit(asyncio.run(run(parse_args())))
