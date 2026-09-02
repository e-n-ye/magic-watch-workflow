import copy
import json
import sys
import unittest
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

from screen_map_to_xml import generate
from screen_map_validate import validate


ROOT = Path(__file__).resolve().parents[3]
FIXTURE = json.loads(
    (ROOT / "products/f411_watch/ui/maps/watchface.json").read_text(encoding="utf-8")
)


class ScreenMapToolsTests(unittest.TestCase):
    def test_fixture_is_valid(self):
        self.assertEqual(validate(FIXTURE), [])

    def test_nested_coordinates_are_checked_against_parent(self):
        document = copy.deepcopy(FIXTURE)
        next(item for item in document["objects"] if item["name"] == "watchface_steps_label")["x"] = 180
        errors = validate(document)
        self.assertTrue(any("watchface_steps_label" in error and "exceeds parent" in error for error in errors))

    def test_sibling_overlap_is_rejected(self):
        document = copy.deepcopy(FIXTURE)
        next(item for item in document["objects"] if item["name"] == "watchface_status")["x"] = 100
        errors = validate(document)
        self.assertTrue(any("watchface_battery" in error and "watchface_status" in error for error in errors))

    def test_generator_emits_only_supported_nodes(self):
        xml = generate(FIXTURE)
        self.assertIn("<lv_label", xml)
        self.assertIn("<lv_obj", xml)
        self.assertIn('<color name="accent" value="0x64D2FF"', xml)
        self.assertIn('style_border_color="#border"', xml)
        self.assertIn("lv_image", xml)
        self.assertNotIn("style=\"", xml)

    def test_invalid_canvas_type_does_not_crash(self):
        document = copy.deepcopy(FIXTURE)
        document["canvas"]["width"] = "240"
        errors = validate(document)
        self.assertTrue(any("canvas width and height" in error for error in errors))

    def test_unknown_parent_and_font_are_rejected(self):
        document = copy.deepcopy(FIXTURE)
        document["objects"][0]["font"] = "custom_99"
        document["objects"][1]["parent"] = "missing"
        errors = validate(document)
        self.assertTrue(any("does not exist" in error for error in errors))
        self.assertTrue(any("unsupported font" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
