#!/usr/bin/env python3
"""Generate a small LVGL Pro Editor XML screen from a confirmed screen map."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from xml.etree import ElementTree as ET

from screen_map_validate import validate


def _attrs(item: dict) -> dict[str, str]:
    attrs = {"name": item["name"], "x": str(item["x"]), "y": str(item["y"]),
             "width": str(item["width"]), "height": str(item["height"]),
             "ignore_layout": "true"}
    if item.get("align"):
        attrs["align"] = item["align"]
    if item.get("text_color"):
        attrs["style_text_color"] = f"#{item['text_color']}"
    if item.get("font") and item["font"] != "default":
        attrs["style_text_font"] = item["font"]
    if item.get("text_align"):
        attrs["style_text_align"] = item["text_align"]
    if item.get("bg_color"):
        attrs["style_bg_color"] = f"#{item['bg_color']}"
    if item.get("border_color"):
        attrs["style_border_color"] = f"#{item['border_color']}"
    for key in ("border_width", "radius", "pad_all"):
        if key in item:
            attrs[f"style_{key}"] = str(item[key])
    if item.get("interactive"):
        attrs["clickable"] = "true"
    return attrs


def generate(document: dict) -> str:
    errors = validate(document)
    if errors:
        raise ValueError("; ".join(errors))
    screen = ET.Element("screen")
    tokens = document.get("tokens", {})
    consts = ET.SubElement(screen, "consts")
    color_tokens = tokens.get("colors", {}) if isinstance(tokens, dict) else {}
    int_tokens = tokens.get("ints", {}) if isinstance(tokens, dict) else {}
    if not isinstance(color_tokens, dict) or not isinstance(int_tokens, dict):
        raise ValueError("tokens.colors and tokens.ints must be objects")
    for name, value in color_tokens.items():
        ET.SubElement(consts, "color", {"name": name, "value": str(value)})
    for name, value in int_tokens.items():
        ET.SubElement(consts, "int", {"name": name, "value": str(value)})
    if len(consts) == 0:
        screen.remove(consts)
    view = ET.SubElement(screen, "view")
    children: dict[str, list[dict]] = {}
    for item in document["objects"]:
        children.setdefault(item.get("parent", "root"), []).append(item)

    def append(parent: ET.Element, item: dict) -> None:
        object_type = item.get("type")
        tag = "lv_label" if object_type == "label" else "lv_image" if object_type == "image" else "lv_obj"
        node = ET.SubElement(parent, tag, _attrs(item))
        if tag == "lv_label":
            node.set("text", item.get("text", ""))
        if tag == "lv_image":
            node.set("src", item["src"])
        for child in children.get(item["name"], []):
            append(node, child)

    for item in children.get("root", []):
        append(view, item)
    ET.indent(screen, space="    ")
    return ET.tostring(screen, encoding="unicode") + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("screen_map", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args(argv)
    document = json.loads(args.screen_map.read_text(encoding="utf-8"))
    args.output.write_text(generate(document), encoding="utf-8", newline="\n")
    print(f"screen-map: wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
