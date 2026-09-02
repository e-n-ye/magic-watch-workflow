#!/usr/bin/env python3
"""Validate a confirmed visual screen map before generating LVGL XML."""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any


NAME_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_]*$")
TOKEN_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_]*$")
FONT_HEIGHTS = {"default": 14, "montserrat_8": 8, "montserrat_14": 14,
                "montserrat_24": 24, "montserrat_40": 40}
SUPPORTED_TYPES = {"label", "object", "image"}


@dataclass(frozen=True)
class Box:
    name: str
    parent: str
    x: int
    y: int
    width: int
    height: int
    allow_overlap: bool = False

    @property
    def right(self) -> int:
        return self.x + self.width

    @property
    def bottom(self) -> int:
        return self.y + self.height


def _error(errors: list[str], message: str) -> None:
    errors.append(message)


def _intersects(first: Box, second: Box) -> bool:
    return first.x < second.right and second.x < first.right and first.y < second.bottom and second.y < first.bottom


def validate(document: dict[str, Any], expected_width: int = 240,
             expected_height: int = 280) -> list[str]:
    errors: list[str] = []
    if not isinstance(document, dict):
        return ["screen-map must be an object"]
    version = document.get("version")
    if not isinstance(version, int) or version < 1:
        _error(errors, "version must be a positive integer")
    screen_name = document.get("screen")
    if not isinstance(screen_name, str) or not NAME_RE.fullmatch(screen_name):
        _error(errors, "screen must be a valid LVGL screen name")
    canvas = document.get("canvas")
    if not isinstance(canvas, dict):
        return ["canvas must be an object"]
    width = canvas.get("width")
    height = canvas.get("height")
    if not isinstance(width, int) or not isinstance(height, int):
        _error(errors, "canvas width and height must be integers")
        width = expected_width
        height = expected_height
    elif width != expected_width or height != expected_height:
        _error(errors, f"canvas must be {expected_width}x{expected_height}, got {width}x{height}")
    safe_area = document.get("safe_area")
    if isinstance(safe_area, dict):
        safe_values = [safe_area.get(key) for key in ("x", "y", "width", "height")]
        if not all(isinstance(value, int) for value in safe_values):
            _error(errors, "safe_area x, y, width and height must be integers")
        elif (safe_area["x"] < 0 or safe_area["y"] < 0 or safe_area["width"] <= 0
              or safe_area["height"] <= 0 or safe_area["x"] + safe_area["width"] > width
              or safe_area["y"] + safe_area["height"] > height):
            _error(errors, "safe_area must be a positive box inside the canvas")
    else:
        _error(errors, "safe_area must be an object")

    tokens = document.get("tokens")
    if tokens is not None:
        if not isinstance(tokens, dict):
            _error(errors, "tokens must be an object")
        else:
            for category in ("colors", "ints"):
                values = tokens.get(category, {})
                if not isinstance(values, dict):
                    _error(errors, f"tokens.{category} must be an object")
                    continue
                for name in values:
                    if not isinstance(name, str) or not TOKEN_RE.fullmatch(name):
                        _error(errors, f"tokens.{category} contains an invalid name: {name!r}")

    objects = document.get("objects")
    if not isinstance(objects, list) or not objects:
        return errors + ["objects must be a non-empty array"]

    by_name: dict[str, dict[str, Any]] = {"root": {"name": "root", "parent": None,
                                                    "x": 0, "y": 0, "width": width,
                                                    "height": height}}
    boxes: dict[str, Box] = {}
    for index, item in enumerate(objects):
        if not isinstance(item, dict):
            _error(errors, f"objects[{index}] must be an object")
            continue
        name = item.get("name")
        if not isinstance(name, str) or not NAME_RE.fullmatch(name):
            _error(errors, f"objects[{index}].name is not a valid LVGL object name")
            continue
        if name in by_name:
            _error(errors, f"duplicate object name: {name}")
            continue
        parent = item.get("parent", "root")
        if not isinstance(parent, str):
            _error(errors, f"{name}: parent must be a string")
            parent = "root"
        object_type = item.get("type", "object")
        if object_type not in SUPPORTED_TYPES:
            _error(errors, f"{name}: unsupported type {object_type!r}")
        if object_type == "image" and (
            not isinstance(item.get("src"), str) or not NAME_RE.fullmatch(item["src"])
        ):
            _error(errors, f"{name}: image objects need a valid src resource name")
        by_name[name] = item
        values = [item.get(key) for key in ("x", "y", "width", "height")]
        if not all(isinstance(value, int) for value in values):
            _error(errors, f"{name}: x, y, width and height must be integers")
            continue
        x, y, obj_width, obj_height = values
        if obj_width <= 0 or obj_height <= 0:
            _error(errors, f"{name}: width and height must be positive")
        boxes[name] = Box(name, parent, x, y, obj_width, obj_height,
                          bool(item.get("allow_overlap", False)))

    def parent_chain(name: str) -> list[str]:
        chain: list[str] = []
        seen: set[str] = set()
        current = name
        while current != "root":
            if current in seen:
                _error(errors, f"{name}: parent cycle detected")
                break
            seen.add(current)
            item = by_name.get(current)
            if item is None:
                _error(errors, f"{name}: parent {current!r} does not exist")
                break
            current = item.get("parent", "root")
            if not isinstance(current, str):
                _error(errors, f"{name}: parent chain contains a non-string value")
                break
            chain.append(current)
        return chain

    for name, box in boxes.items():
        parent_chain(name)
        parent = boxes.get(box.parent)
        if box.parent == "root":
            parent_width, parent_height = width, height
        elif parent is not None:
            parent_width, parent_height = parent.width, parent.height
        else:
            continue
        if box.x < 0 or box.y < 0 or box.right > parent_width or box.bottom > parent_height:
            _error(errors, f"{name}: box ({box.x},{box.y},{box.width},{box.height}) exceeds parent {box.parent}")
        text = by_name[name].get("text")
        if isinstance(text, str):
            font = by_name[name].get("font", "default")
            if font not in FONT_HEIGHTS:
                _error(errors, f"{name}: unsupported font {font!r}")
                font_height = FONT_HEIGHTS["default"]
            else:
                font_height = FONT_HEIGHTS[font]
            estimated_width = max((len(line) for line in text.split("\n")), default=0) * font_height * 62 // 100
            if estimated_width > box.width:
                _error(errors, f"{name}: text estimate {estimated_width}px exceeds width {box.width}px")

        for color_key in ("text_color", "bg_color", "border_color"):
            color = by_name[name].get(color_key)
            if color is not None and (not isinstance(color, str) or not TOKEN_RE.fullmatch(color)):
                _error(errors, f"{name}: {color_key} must be a color token name")

        hitbox = by_name[name].get("hitbox") if by_name[name].get("interactive") else None
        if hitbox is not None:
            if (not isinstance(hitbox, dict) or not isinstance(hitbox.get("width"), int)
                    or not isinstance(hitbox.get("height"), int)
                    or hitbox.get("width", 0) < 24 or hitbox.get("height", 0) < 24):
                _error(errors, f"{name}: hitbox must be at least 24x24")

    children: dict[str, list[Box]] = {}
    for box in boxes.values():
        children.setdefault(box.parent, []).append(box)
    for parent_name, siblings in children.items():
        for index, first in enumerate(siblings):
            for second in siblings[index + 1:]:
                if _intersects(first, second) and not (first.allow_overlap or second.allow_overlap):
                    _error(errors, f"siblings {first.name} and {second.name} overlap in {parent_name}")
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("screen_map", type=Path)
    parser.add_argument("--expected-width", type=int, default=240)
    parser.add_argument("--expected-height", type=int, default=280)
    args = parser.parse_args(argv)
    try:
        document = json.loads(args.screen_map.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"screen-map: cannot read {args.screen_map}: {exc}", file=sys.stderr)
        return 2
    errors = validate(document, args.expected_width, args.expected_height)
    if errors:
        for error in errors:
            print(f"screen-map: ERROR: {error}", file=sys.stderr)
        return 1
    print(f"screen-map: PASS {args.screen_map} objects={len(document['objects'])} canvas=240x280")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
