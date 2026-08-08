# F411 signed image package

`manifest.py` is the host-side packer and verifier for the F411 application
slot. It consumes the raw `my_watch_f411.bin` output, never parses XML or ELF,
and writes a complete 448 KiB application slot image:

```text
0x00000 .. 0x6EFFF  application bytes and 0xFF padding
0x6F000 .. 0x6FFFF  4 KiB manifest trailer
```

The trailer uses explicit little-endian fields at fixed offsets:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 4 | magic `MWMF` |
| 4 | 2 | format version (`1`) |
| 6 | 2 | signed header size (`128`) |
| 8 | 4 | board id (`F411` as a little-endian integer) |
| 12 | 4 | firmware version |
| 16 | 4 | security counter |
| 20 | 4 | load address (`0x08010000`) |
| 24 | 4 | image length |
| 28 | 32 | SHA-256 of the image bytes |
| 60 | 1 | public key id (`0`) |
| 61 | 3 | reserved, zero |
| 64 | 64 | ECDSA P-256 signature, raw `r || s` |
| 128 | 3968 | erased padding (`0xFF`) |

The signature covers bytes `0..63` of the header. The host invokes OpenSSL for
ECDSA P-256 and converts its DER signature to the fixed 64-byte representation.
The private key is supplied by path and is never stored in the repository or
printed by the tool.

Example:

```sh
python3 tools/manifest/manifest.py pack \
  --image firmware/stm32/f411_watch/build/Debug/my_watch_f411.bin \
  --private-key /path/outside/repository/f411-signing-key.pem \
  --firmware-version 1 --security-counter 1 \
  --output build/f411-app-package.bin

python3 tools/manifest/manifest.py verify \
  --package build/f411-app-package.bin \
  --public-key /path/outside/repository/f411-signing-public.pem
```

The VS Code `F411: Pack Signed App (host)` task prompts for the external
private-key path, firmware version, and security counter. The private key must
match the public key compiled into the Bootloader; generating an unrelated key
will produce a package that the board correctly rejects.

The package is not a production OTA protocol yet. It defines the signed image
contract used by the Bootloader; download, metadata journaling, trial boot and
rollback are later rounds.
