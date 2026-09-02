#!/usr/bin/env python3
"""Create the small transparent monochrome PNG inputs used by the LVGL XML project."""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path


ICON_SIZE = 16
ICONS = {
    "icon_battery.png": (
        "................",
        ".############..#",
        ".#..........#..#",
        ".#..........#..#",
        ".#..######..#.##",
        ".#..######..#..#",
        ".#..######..#..#",
        ".#..######..#.##",
        ".#..######..#..#",
        ".#..######..#..#",
        ".#..######..#.##",
        ".#..........#..#",
        ".############..#",
        "................",
        "................",
        "................",
    ),
    "icon_steps.png": (
        "................",
        ".....##.........",
        "....####........",
        "....####........",
        ".....##.........",
        "................",
        ".........##.....",
        "........####....",
        "........####....",
        ".........##.....",
        "................",
        "...##...........",
        "..####..........",
        "..####..........",
        "...##...........",
        "................",
    ),
    "icon_status.png": (
        "................",
        "................",
        ".##.............",
        ".##.............",
        ".##...######....",
        ".##...#....#....",
        ".##...#....#....",
        ".##...######....",
        ".##.............",
        ".##.............",
        ".###########....",
        "................",
        "................",
        "................",
        "................",
        "................",
    ),
    "icon_timer.png": (
        "................",
        "......####......",
        "......####......",
        "................",
        "....########....",
        "...##......##...",
        "...##...#..##...",
        "...##...#..##...",
        "...##..###.##...",
        "...##......##...",
        "...##......##...",
        "....########....",
        "................",
        "................",
        "................",
        "................",
    ),
    "icon_calendar.png": (
        "................",
        "...##......##...",
        "...##......##...",
        ".##############.",
        ".#............#.",
        ".#..##..##..###.",
        ".#............#.",
        ".#..##..##..##..",
        ".#............#.",
        ".#..##..##..##..",
        ".#............#.",
        ".##############.",
        "................",
        "................",
        "................",
        "................",
    ),
    "icon_settings.png": (
        "................",
        ".......##.......",
        "....##.##.##....",
        ".....##..##.....",
        "...##.####.##...",
        "...#..####..#...",
        "..##..####..##..",
        "..##..####..##..",
        "...#..####..#...",
        "...##.####.##...",
        ".....##..##.....",
        "....##.##.##....",
        ".......##.......",
        "................",
        "................",
        "................",
    ),
}


def _chunk(name: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + name + data + struct.pack(">I", zlib.crc32(name + data) & 0xFFFFFFFF)


def _png(rows: tuple[str, ...]) -> bytes:
    if len(rows) != ICON_SIZE or any(len(row) != ICON_SIZE for row in rows):
        raise ValueError("every icon must be a 16x16 pixel map")

    raw = bytearray()
    for row in rows:
        raw.append(0)
        for offset in range(0, ICON_SIZE, 8):
            value = 0
            for pixel in row[offset : offset + 8]:
                value = (value << 1) | (1 if pixel == "#" else 0)
            raw.append(value)
    return b"".join(
        (
            b"\x89PNG\r\n\x1a\n",
            _chunk(b"IHDR", struct.pack(">IIBBBBB", ICON_SIZE, ICON_SIZE, 1, 3, 0, 0, 0)),
            _chunk(b"PLTE", bytes((0, 0, 0, 244, 247, 250))),
            _chunk(b"tRNS", bytes((0, 255))),
            _chunk(b"IDAT", zlib.compress(bytes(raw), level=9)),
            _chunk(b"IEND", b""),
        )
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    if not args.check:
        args.output_dir.mkdir(parents=True, exist_ok=True)
    for name, rows in ICONS.items():
        path = args.output_dir / name
        expected = _png(rows)
        if args.check:
            if not path.is_file() or path.read_bytes() != expected:
                raise SystemExit(f"icon asset is missing or out of date: {path}")
        else:
            path.write_bytes(expected)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
