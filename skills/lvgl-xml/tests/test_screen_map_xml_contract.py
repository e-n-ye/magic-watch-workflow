import copy
import json
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

from validate_screen_map_xml import validate


ROOT = Path(__file__).resolve().parents[3]
UI = ROOT / "products/f411_watch/ui"
PAGES = ("watchface", "launcher", "status", "settings", "timer", "calendar")


class ScreenMapXmlContractTests(unittest.TestCase):
    def test_all_v1_maps_match_their_xml(self):
        for page in PAGES:
            with self.subTest(page=page):
                self.assertEqual(
                    validate(UI / "maps" / f"{page}.json", UI / "screens" / page / f"screen_{page}.xml", UI / "globals.xml"),
                    [],
                )

    def test_coordinate_drift_is_rejected(self):
        source = UI / "maps" / "launcher.json"
        document = json.loads(source.read_text(encoding="utf-8"))
        next(item for item in document["objects"] if item["name"] == "launcher_item_timer")["y"] = 45
        with tempfile.TemporaryDirectory() as directory:
            changed_map = Path(directory) / "launcher.json"
            changed_map.write_text(json.dumps(copy.deepcopy(document)), encoding="utf-8")
            errors = validate(changed_map, UI / "screens/launcher/screen_launcher.xml", UI / "globals.xml")
        self.assertTrue(any("launcher_item_timer.y" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
