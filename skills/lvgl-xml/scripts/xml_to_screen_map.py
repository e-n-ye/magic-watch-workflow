#!/usr/bin/env python3
"""Extract the geometry contract from a completed LVGL Pro Editor XML screen."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from xml.etree import ElementTree as ET


TYPE_BY_TAG = {"lv_label": "label", "lv_obj": "object", "lv_image": "image"}
GEOMETRY = ("x", "y", "width", "height")


def _constants(globals_path: Path) -> dict[str, int]:
    root = ET.parse(globals_path).getroot()
    return {
        node.get("name"): int(node.get("value"))
        for node in root.findall("./consts/int") + root.findall("./consts/px")
        if node.get("name") and node.get("value")
    }


def _integer(value: str | None, constants: dict[str, int], name: str, attr: str) -> int:
    if value is None:
        raise ValueError(f"{name}: missing {attr}")
    if value.startswith("#"):
        value = str(constants.get(value[1:], ""))
    try:
        return int(value, 10)
    except ValueError as exc:
        raise ValueError(f"{name}: {attr} is not a concrete integer: {value!r}") from exc


def extract(xml_path: Path, globals_path: Path) -> dict:
    root = ET.parse(xml_path).getroot()
    view = root.find("view")
    if view is None:
        raise ValueError("screen has no view")
    constants = _constants(globals_path)
    objects: list[dict] = []

    def visit(parent: ET.Element, parent_name: str) -> None:
        for node in parent:
            object_type = TYPE_BY_TAG.get(node.tag)
            if object_type is None:
                continue
            name = node.get("name")
            if not name:
                raise ValueError(f"unnamed {node.tag} under {parent_name}")
            item = {
                "name": name,
                "type": object_type,
                "parent": parent_name,
                **{attr: _integer(node.get(attr), constants, name, attr) for attr in GEOMETRY},
            }
            if object_type == "label":
                item["text"] = node.get("text", "")
                item["font"] = node.get("style_text_font", "default")
            if object_type == "image":
                item["src"] = node.get("src", "")
            objects.append(item)
            visit(node, name)

    visit(view, "root")
    return {
        "version": 1,
        "screen": xml_path.parent.name,
        "canvas": {"width": 240, "height": 280},
        "safe_area": {"x": 16, "y": 8, "width": 208, "height": 264},
        "objects": objects,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xml", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--globals", required=True, type=Path)
    args = parser.parse_args()
    result = extract(args.xml, args.globals)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
