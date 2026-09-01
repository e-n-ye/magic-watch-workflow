import tempfile
import unittest
from pathlib import Path
import sys


SCRIPT_DIR = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

from validate_xml_resources import validate


ROOT = Path(__file__).resolve().parents[3]
GLOBALS = ROOT / "products/f411_watch/ui/globals.xml"
LV_CONF = ROOT / "firmware/stm32/f411_watch/user/ui/lv_conf.h"
WATCHFACE = ROOT / "products/f411_watch/ui/screens/watchface/screen_watchface.xml"


class XmlResourceTests(unittest.TestCase):
    def test_watchface_resources_are_registered(self):
        self.assertEqual(validate(WATCHFACE, GLOBALS, LV_CONF), [])

    def test_missing_font_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "screen.xml"
            path.write_text(
                '<screen><view><lv_label name="title" text="X" '
                'style_text_font="missing_99" /></view></screen>',
                encoding="utf-8",
            )
            errors = validate(path, GLOBALS, LV_CONF)
            self.assertTrue(any("not registered" in error for error in errors))

    def test_disabled_image_widget_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "screen.xml"
            path.write_text(
                '<screen><view><lv_image name="icon" src="missing" /></view></screen>',
                encoding="utf-8",
            )
            errors = validate(path, GLOBALS, LV_CONF)
            self.assertTrue(any("LV_USE_IMAGE=0" in error for error in errors))

    def test_duplicate_widget_names_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "screen.xml"
            path.write_text(
                '<screen><view><lv_label name="title" text="A" />'
                '<lv_label name="title" text="B" /></view></screen>',
                encoding="utf-8",
            )
            errors = validate(path, GLOBALS, LV_CONF)
            self.assertTrue(any("duplicate object" in error for error in errors))

    def test_remove_style_all_after_geometry_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "screen.xml"
            path.write_text(
                '<screen><view><lv_obj name="card" x="16" y="20" '
                'width="208" height="58"><remove_style_all /></lv_obj></view></screen>',
                encoding="utf-8",
            )
            errors = validate(path, GLOBALS, LV_CONF)
            self.assertTrue(any("clears those local style properties" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
