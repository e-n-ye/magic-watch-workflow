#!/usr/bin/env python3
"""Host tests for the fixed-offset F411 signed package format."""

from __future__ import annotations

import subprocess
import tempfile
import unittest
from pathlib import Path

from manifest import (
    APP_SLOT_SIZE,
    HEADER_SIZE,
    MAX_IMAGE_LENGTH,
    TRAILER_OFFSET,
    TRAILER_SIZE,
    ManifestError,
    pack_image,
    verify_package,
)


class ManifestTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory(prefix="f411-manifest-test-")
        self.root = Path(self.temp_dir.name)
        self.image_path = self.root / "app.bin"
        self.private_key = self.root / "private.pem"
        self.public_key = self.root / "public.pem"
        self.package_path = self.root / "app-package.bin"
        self.image_path.write_bytes(bytes(range(256)) * 8)
        self._run_openssl("ecparam", "-name", "prime256v1", "-genkey", "-noout", "-out", self.private_key)
        self._run_openssl("ec", "-in", self.private_key, "-pubout", "-out", self.public_key)
        pack_image(self.image_path, self.private_key, self.package_path, 7, 3)

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def _run_openssl(self, *arguments: object) -> None:
        subprocess.run(
            ["openssl", *[str(argument) for argument in arguments]],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

    def _package_bytes(self) -> bytearray:
        return bytearray(self.package_path.read_bytes())

    def _write_package(self, data: bytearray) -> None:
        self.package_path.write_bytes(data)

    def test_valid_package(self) -> None:
        fields = verify_package(self.package_path, self.public_key)
        self.assertEqual(fields["firmware_version"], 7)
        self.assertEqual(fields["security_counter"], 3)
        self.assertEqual(fields["image_length"], 2048)
        self.assertEqual(self.package_path.stat().st_size, APP_SLOT_SIZE)

    def test_fixed_trailer_and_erased_padding(self) -> None:
        package = self._package_bytes()
        self.assertEqual(package[TRAILER_OFFSET + HEADER_SIZE], 0xFF)
        self.assertEqual(len(package[TRAILER_OFFSET : TRAILER_OFFSET + TRAILER_SIZE]), TRAILER_SIZE)

    def test_corrupt_image_is_rejected(self) -> None:
        package = self._package_bytes()
        package[10] ^= 0x01
        self._write_package(package)
        with self.assertRaises(ManifestError):
            verify_package(self.package_path, self.public_key)

    def test_wrong_board_is_rejected_before_signature(self) -> None:
        package = self._package_bytes()
        package[TRAILER_OFFSET + 8] ^= 0x01
        self._write_package(package)
        with self.assertRaisesRegex(ManifestError, "board mismatch"):
            verify_package(self.package_path, self.public_key)

    def test_out_of_range_length_is_rejected(self) -> None:
        package = self._package_bytes()
        package[TRAILER_OFFSET + 24 : TRAILER_OFFSET + 28] = (MAX_IMAGE_LENGTH + 1).to_bytes(4, "little")
        self._write_package(package)
        with self.assertRaisesRegex(ManifestError, "out of range"):
            verify_package(self.package_path, self.public_key)

    def test_unsigned_package_is_rejected(self) -> None:
        package = self._package_bytes()
        package[TRAILER_OFFSET + 64 : TRAILER_OFFSET + 128] = b"\x00" * 64
        self._write_package(package)
        with self.assertRaisesRegex(ManifestError, "signature is empty"):
            verify_package(self.package_path, self.public_key)

    def test_bad_padding_is_rejected(self) -> None:
        package = self._package_bytes()
        package[TRAILER_OFFSET + TRAILER_SIZE - 1] = 0
        self._write_package(package)
        with self.assertRaisesRegex(ManifestError, "padding"):
            verify_package(self.package_path, self.public_key)

    def test_non_erased_image_padding_is_rejected(self) -> None:
        package = self._package_bytes()
        package[2048] = 0
        self._write_package(package)
        with self.assertRaisesRegex(ManifestError, "non-erased bytes"):
            verify_package(self.package_path, self.public_key)


if __name__ == "__main__":
    unittest.main()
