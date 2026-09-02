#!/usr/bin/env python3
"""Require the screen-map metadata and LVGL XML geometry to stay identical."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from xml.etree import ElementTree as ET

from screen_map_validate import validate as validate_map


TAGS = {"label": "lv_label", "object": "lv_obj", "image": "lv_image"}
GEOMETRY = ("x", "y", "width", "height")


def _constants(globals_path: Path) -> dict[str, str]:
    root = ET.parse(globals_path).getroot()
    return {
        node.get("name"): node.get("value")
        for node in root.findall("./consts/int") + root.findall("./consts/px")
        if node.get("name") and node.get("value")
    }


def _value(value: str | None, constants: dict[str, str]) -> int | None:
    if value is None:
        return None
    if value.startswith("#"):
        value = constants.get(value[1:], "")
    try:
        return int(value, 10)
    except ValueError:
        return None


def validate(screen_map_path: Path, xml_path: Path, globals_path: Path) -> list[str]:
    document = json.loads(screen_map_path.read_text(encoding="utf-8"))
    errors = validate_map(document)
    if errors:
        return errors
    root = ET.parse(xml_path).getroot()
    view = root.find("view")
    if view is None:
        return [f"{xml_path.name}: missing view"]
    constants = _constants(globals_path)
    map_objects = {item["name"]: item for item in document["objects"]}
    xml_objects: dict[str, tuple[ET.Element, str]] = {}

    def collect(parent: ET.Element, parent_name: str) -> None:
        for node in parent:
            if node.tag not in TAGS.values():
                continue
            name = node.get("name")
            if not name:
                errors.append(f"{xml_path.name}: unnamed {node.tag} is outside the map contract")
                continue
            if name in xml_objects:
                errors.append(f"{xml_path.name}: duplicate named object {name!r}")
                continue
            xml_objects[name] = (node, parent_name)
            collect(node, name)

    collect(view, "root")
    for name, item in map_objects.items():
        entry = xml_objects.get(name)
        if entry is None:
            errors.append(f"{xml_path.name}: map object {name!r} is missing from XML")
            continue
        node, parent_name = entry
        expected_tag = TAGS[item.get("type", "object")]
        if node.tag != expected_tag:
            errors.append(f"{xml_path.name}: {name} uses {node.tag}, expected {expected_tag}")
        if parent_name != item.get("parent", "root"):
            errors.append(f"{xml_path.name}: {name} parent {parent_name!r} does not match map")
        for attr in GEOMETRY:
            expected = item[attr]
            actual = _value(node.get(attr), constants)
            if actual != expected:
                errors.append(f"{xml_path.name}: {name}.{attr} is {node.get(attr)!r}, expected {expected}")
    for name in xml_objects:
        if name not in map_objects:
            errors.append(f"{xml_path.name}: XML object {name!r} is absent from the map")
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("screen_map", type=Path)
    parser.add_argument("xml", type=Path)
    parser.add_argument("--globals", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        errors = validate(args.screen_map, args.xml, args.globals)
    except (OSError, json.JSONDecodeError, ET.ParseError) as exc:
        print(f"screen-map-xml: cannot read input: {exc}", file=sys.stderr)
        return 2
    if errors:
        for error in errors:
            print(f"screen-map-xml: ERROR: {error}", file=sys.stderr)
        return 1
    print(f"screen-map-xml: PASS {args.screen_map} == {args.xml}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
