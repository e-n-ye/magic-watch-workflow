#!/usr/bin/env python3
"""Check XML resource references against globals.xml and the F411 LVGL config."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from xml.etree import ElementTree as ET


def validate(xml_path: Path, globals_path: Path, lv_conf_path: Path) -> list[str]:
    errors: list[str] = []
    xml_root = ET.parse(xml_path).getroot()
    globals_root = ET.parse(globals_path).getroot()
    fonts = {node.get("name") for node in globals_root.findall("./fonts/*") if node.get("name")}
    colors = {node.get("name") for node in globals_root.findall("./consts/color") if node.get("name")}
    images = {node.get("name") for node in globals_root.findall("./images/*") if node.get("name")}
    conf = lv_conf_path.read_text(encoding="utf-8")
    names: set[str] = set()
    object_tags = {"lv_obj", "lv_label", "lv_image", "lv_imagebutton", "lv_img"}
    for node in xml_root.iter():
        name = node.get("name")
        if name and node.tag in object_tags:
            if name in names:
                errors.append(f"{xml_path.name}: duplicate object/resource name {name!r}")
            names.add(name)
        if node.tag == "view" and node.get("style"):
            errors.append(f"{xml_path.name}: use a child <style> element, not view style=...")

    for node in globals_root.findall("./fonts/*") + globals_root.findall("./images/*"):
        source = node.get("src_path")
        if source and not (globals_path.parent / source).is_file():
            errors.append(f"{globals_path.name}: resource {source!r} does not exist")

    image_enabled = re.search(r"#define\s+LV_USE_IMAGE\s+([01])\b", conf)
    image_widgets = {"lv_image", "lv_imagebutton", "lv_img"}
    if image_enabled and image_enabled.group(1) == "0":
        for node in xml_root.iter():
            if node.tag in image_widgets:
                errors.append(f"{xml_path.name}: image widget used while LV_USE_IMAGE=0")

    for node in xml_root.iter():
        font = node.get("style_text_font")
        if font and font not in fonts and font != "default":
            errors.append(f"{xml_path.name}: font {font!r} is not registered in globals.xml")
            continue
        if font and font.startswith("montserrat_"):
            size = font.removeprefix("montserrat_")
            if not re.search(rf"#define\s+LV_FONT_MONTSERRAT_{re.escape(size)}\s+1\b", conf) and not re.search(r"#define\s+WATCH_UI_FONT_ALIAS_BUDGETED\s+1\b", conf):
                errors.append(f"{xml_path.name}: {font} is not enabled in lv_conf.h")
        for attr in ("style_text_color", "style_bg_color", "style_border_color"):
            value = node.get(attr)
            if value and value.startswith("#") and value[1:] not in colors:
                errors.append(f"{xml_path.name}: color {value!r} is not declared in globals.xml")
        image = node.get("src") or node.get("src_image")
        if image and image not in images:
            errors.append(f"{xml_path.name}: image {image!r} is not declared in globals.xml")
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xml", type=Path)
    parser.add_argument("--globals", required=True, type=Path)
    parser.add_argument("--lv-conf", required=True, type=Path)
    args = parser.parse_args(argv)
    errors = validate(args.xml, args.globals, args.lv_conf)
    if errors:
        for error in errors:
            print(f"xml-resources: ERROR: {error}", file=sys.stderr)
        return 1
    print(f"xml-resources: PASS {args.xml}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
