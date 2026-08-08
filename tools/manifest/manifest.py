#!/usr/bin/env python3
"""Pack and verify signed F411 application images.

The package format is deliberately small and fixed-offset so the Bootloader
can parse it without mapping a packed C structure.  OpenSSL is used only by
the host tool; the private key is read from the path supplied by the caller.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

MAGIC = b"MWMF"
FORMAT_VERSION = 1
HEADER_SIZE = 128
TRAILER_SIZE = 4096
SIGNED_HEADER_SIZE = 64
SIGNATURE_SIZE = 64
APP_START = 0x08010000
APP_EXECUTION_END = 0x0807F000
APP_SLOT_END = 0x08080000
APP_SLOT_SIZE = APP_SLOT_END - APP_START
TRAILER_OFFSET = APP_EXECUTION_END - APP_START
MAX_IMAGE_LENGTH = APP_EXECUTION_END - APP_START
BOARD_ID = int.from_bytes(b"F411", "little")
KEY_ID = 0

# Order of secp256r1, used to normalize ECDSA signatures to low-S form.
P256_ORDER = int(
    "FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551", 16
)


class ManifestError(ValueError):
    """Raised when a package or manifest violates the format contract."""


def _der_length(length: int) -> bytes:
    if length < 0x80:
        return bytes([length])
    encoded = length.to_bytes((length.bit_length() + 7) // 8, "big")
    return bytes([0x80 | len(encoded)]) + encoded


def _der_integer(value: int) -> bytes:
    if value <= 0:
        raise ManifestError("ECDSA integer must be positive")
    encoded = value.to_bytes((value.bit_length() + 7) // 8, "big")
    if encoded[0] & 0x80:
        encoded = b"\x00" + encoded
    return b"\x02" + _der_length(len(encoded)) + encoded


def _raw_signature_to_der(signature: bytes) -> bytes:
    if len(signature) != SIGNATURE_SIZE:
        raise ManifestError("signature must be 64 bytes")
    r = int.from_bytes(signature[:32], "big")
    s = int.from_bytes(signature[32:], "big")
    return b"\x30" + _der_length(len(_der_integer(r)) + len(_der_integer(s))) + _der_integer(r) + _der_integer(s)


def _read_der_length(data: bytes, offset: int) -> tuple[int, int]:
    if offset >= len(data):
        raise ManifestError("truncated DER length")
    first = data[offset]
    offset += 1
    if first < 0x80:
        return first, offset
    count = first & 0x7F
    if count == 0 or count > 2 or offset + count > len(data):
        raise ManifestError("invalid DER length")
    return int.from_bytes(data[offset : offset + count], "big"), offset + count


def _der_to_raw_signature(data: bytes) -> bytes:
    if not data or data[0] != 0x30:
        raise ManifestError("OpenSSL returned a non-DER ECDSA signature")
    sequence_length, offset = _read_der_length(data, 1)
    if offset + sequence_length != len(data):
        raise ManifestError("invalid DER sequence length")

    values = []
    for _ in range(2):
        if offset >= len(data) or data[offset] != 0x02:
            raise ManifestError("invalid DER ECDSA integer")
        length, value_offset = _read_der_length(data, offset + 1)
        end = value_offset + length
        if end > len(data) or length == 0:
            raise ManifestError("truncated DER ECDSA integer")
        encoded = data[value_offset:end]
        if encoded[0] & 0x80:
            raise ManifestError("negative DER ECDSA integer")
        values.append(int.from_bytes(encoded, "big"))
        offset = end

    r, s = values
    if not 0 < r < P256_ORDER or not 0 < s < P256_ORDER:
        raise ManifestError("ECDSA integer is outside secp256r1 order")
    if s > P256_ORDER // 2:
        s = P256_ORDER - s
    return r.to_bytes(32, "big") + s.to_bytes(32, "big")


def _openssl_sign(message: bytes, private_key: Path) -> bytes:
    result = subprocess.run(
        ["openssl", "dgst", "-sha256", "-sign", str(private_key)],
        input=message,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode != 0:
        raise ManifestError("OpenSSL could not sign the manifest")
    return _der_to_raw_signature(result.stdout)


def _openssl_verify(message: bytes, signature: bytes, public_key: Path) -> bool:
    signature_path = None
    try:
        with tempfile.NamedTemporaryFile(
            prefix="f411-signature-", suffix=".der", delete=False
        ) as signature_file:
            signature_file.write(_raw_signature_to_der(signature))
            signature_path = Path(signature_file.name)
        result = subprocess.run(
            [
                "openssl",
                "dgst",
                "-sha256",
                "-verify",
                str(public_key),
                "-signature",
                str(signature_path),
            ],
            input=message,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
        return result.returncode == 0 and result.stdout.strip() == b"Verified OK"
    finally:
        if signature_path is not None:
            signature_path.unlink(missing_ok=True)


def _read_u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def _read_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def _write_u16(data: bytearray, offset: int, value: int) -> None:
    struct.pack_into("<H", data, offset, value)


def _write_u32(data: bytearray, offset: int, value: int) -> None:
    struct.pack_into("<I", data, offset, value)


def _validate_header(header: bytes, expected_board: int = BOARD_ID) -> None:
    if len(header) < HEADER_SIZE:
        raise ManifestError("trailer is shorter than the manifest header")
    if header[:4] != MAGIC:
        raise ManifestError("manifest magic mismatch")
    if _read_u16(header, 4) != FORMAT_VERSION:
        raise ManifestError("unsupported manifest format")
    if _read_u16(header, 6) != HEADER_SIZE:
        raise ManifestError("unexpected manifest header size")
    if _read_u32(header, 8) != expected_board:
        raise ManifestError("manifest board mismatch")
    if _read_u32(header, 20) != APP_START:
        raise ManifestError("manifest load address mismatch")
    image_length = _read_u32(header, 24)
    if image_length == 0 or image_length > MAX_IMAGE_LENGTH:
        raise ManifestError("manifest image length is out of range")
    if header[60] != KEY_ID:
        raise ManifestError("unknown manifest key id")
    if header[61:64] != b"\x00\x00\x00":
        raise ManifestError("manifest reserved bytes are not zero")
    if header[64:128] == b"\x00" * SIGNATURE_SIZE:
        raise ManifestError("manifest signature is empty")


def _parse_package(package: bytes, expected_board: int = BOARD_ID) -> tuple[bytes, bytes]:
    if len(package) != APP_SLOT_SIZE:
        raise ManifestError(f"package must be exactly {APP_SLOT_SIZE} bytes")
    trailer = package[TRAILER_OFFSET : TRAILER_OFFSET + TRAILER_SIZE]
    header = trailer[:HEADER_SIZE]
    _validate_header(header, expected_board)
    if trailer[HEADER_SIZE:] != b"\xFF" * (TRAILER_SIZE - HEADER_SIZE):
        raise ManifestError("manifest trailer padding is not erased")
    image_length = _read_u32(header, 24)
    if package[image_length:TRAILER_OFFSET] != b"\xFF" * (TRAILER_OFFSET - image_length):
        raise ManifestError("package has non-erased bytes before the trailer")
    return package[:image_length], trailer


def build_header(image: bytes, firmware_version: int, security_counter: int) -> bytes:
    if not 0 <= firmware_version <= 0xFFFFFFFF:
        raise ManifestError("firmware version must fit in uint32")
    if not 0 <= security_counter <= 0xFFFFFFFF:
        raise ManifestError("security counter must fit in uint32")
    if not 0 < len(image) <= MAX_IMAGE_LENGTH:
        raise ManifestError("application image length is out of range")

    header = bytearray(HEADER_SIZE)
    header[:4] = MAGIC
    _write_u16(header, 4, FORMAT_VERSION)
    _write_u16(header, 6, HEADER_SIZE)
    _write_u32(header, 8, BOARD_ID)
    _write_u32(header, 12, firmware_version)
    _write_u32(header, 16, security_counter)
    _write_u32(header, 20, APP_START)
    _write_u32(header, 24, len(image))
    header[28:60] = hashlib.sha256(image).digest()
    header[60] = KEY_ID
    return bytes(header)


def pack_image(image_path: Path, private_key: Path, output_path: Path, firmware_version: int, security_counter: int) -> None:
    if not private_key.is_file():
        raise ManifestError("private key path must name an existing file")
    image = image_path.read_bytes()
    header = bytearray(build_header(image, firmware_version, security_counter))
    header[64:128] = _openssl_sign(bytes(header[:SIGNED_HEADER_SIZE]), private_key)
    trailer = bytes(header) + b"\xFF" * (TRAILER_SIZE - HEADER_SIZE)
    package = image + b"\xFF" * (TRAILER_OFFSET - len(image)) + trailer
    if len(package) != APP_SLOT_SIZE:
        raise ManifestError("internal package size calculation failed")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(package)


def verify_package(package_path: Path, public_key: Path, expected_board: int = BOARD_ID) -> dict[str, int]:
    package = package_path.read_bytes()
    image, trailer = _parse_package(package, expected_board)
    header = trailer[:HEADER_SIZE]
    if hashlib.sha256(image).digest() != header[28:60]:
        raise ManifestError("application SHA-256 mismatch")
    if not _openssl_verify(header[:SIGNED_HEADER_SIZE], header[64:128], public_key):
        raise ManifestError("ECDSA signature verification failed")
    return {
        "board_id": _read_u32(header, 8),
        "firmware_version": _read_u32(header, 12),
        "security_counter": _read_u32(header, 16),
        "image_length": _read_u32(header, 24),
        "key_id": header[60],
    }


def _parse_int(value: str) -> int:
    return int(value, 0)


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    pack_parser = subparsers.add_parser("pack", help="sign a raw application image")
    pack_parser.add_argument("--image", type=Path, required=True)
    pack_parser.add_argument("--private-key", type=Path, required=True)
    pack_parser.add_argument("--output", type=Path, required=True)
    pack_parser.add_argument("--firmware-version", type=_parse_int, required=True)
    pack_parser.add_argument("--security-counter", type=_parse_int, required=True)

    verify_parser = subparsers.add_parser("verify", help="verify a signed application package")
    verify_parser.add_argument("--package", type=Path, required=True)
    verify_parser.add_argument("--public-key", type=Path, required=True)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = _build_parser().parse_args(argv)
    try:
        if args.command == "pack":
            pack_image(
                args.image,
                args.private_key,
                args.output,
                args.firmware_version,
                args.security_counter,
            )
            print(f"packed {args.output} ({APP_SLOT_SIZE} bytes)")
        else:
            fields = verify_package(args.package, args.public_key)
            print(
                "verified board=F411 version={firmware_version} counter={security_counter} "
                "image_length={image_length} key_id={key_id}".format(**fields)
            )
        return 0
    except (ManifestError, OSError, subprocess.SubprocessError) as error:
        print(f"manifest error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
