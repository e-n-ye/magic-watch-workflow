# F411 Development Status

This is the rolling status page for the STM32F411 reference firmware. It is
kept short on purpose: Git commits and pull requests contain the historical
record, while this page describes the current project state and next closed
loop.

## End state

The project will provide a real F411 watch reference firmware with a simple
240x280 UI, diagnostics, power-state handling, external resource storage, and
signed OTA with trial boot and rollback. It is a workflow and architecture
reference, not a production dual-chip watch product.

The implementation order is hardware facts, pure-C contracts and host tests,
board integration, UI/resources, power, and secure OTA. Empty placeholder
modules are not part of the project.

## Frozen decisions

| Area | Decision |
| --- | --- |
| F411 display | ST7789, 240x280; 240x240 is a future independent LilyGo profile |
| Core | Pure C `watch_core`; no HAL, FreeRTOS, or LVGL dependency |
| UI ownership | One LVGL UI task owns LVGL and the core; services use bounded queues |
| XML | XML is the layout source and generated C is committed; F411 does not parse XML at runtime |
| Boot layout | Bootloader `0x08000000-0x0800FFFF`; application `0x08010000-0x0807FFFF` |
| App budget | 448 KiB slot with a 4 KiB signed trailer; linked image budget is 400 KiB |
| External flash | W25Q128 metadata, candidate, rollback, and littlefs partitions are separate |
| OTA security | SHA-256 plus ECDSA P-256 signature, board/version checks, trial confirmation, rollback |
| Bluetooth | KT6368 is a transparent SPP/UART transport; protocol and validation stay on F411/host |
| USB | CDC for diagnostics, logs, and controlled resource transfer; no online MSC |
| Security boundary | No claim of protection from SWD/RDP-disabled physical modification |

## Baseline

- Reference commit: `e835ef2` (M1 interrupt-restore fix merged to `main`).
- M0 and M1 CI Gates passed; the current M2 branch starts from this merged
  baseline.
- Before relocation, the verified Debug App image was Flash `64,340 / 524,288 B`
  and RAM `30,208 / 131,072 B`.
- Current M2 build: Bootloader Flash is about `6,564 / 65,536 B`; the App is
  `64,356 / 454,656 B` before the package trailer.
- Current checks passed: Debug configure, Debug build, `format-check`,
  `cppcheck`, ELF section/vector inspection, linker boundary assertions, and
  the signed-package host tests.
- Current hardware facts: STM32F411, 24 MHz HSE, ST-Link-compatible SWD,
  ST7789 display, CST816 touch, W25Q128, and KT6368 UART wiring.

## Milestones

| ID | Scope | Status |
| --- | --- | --- |
| M0 | Rolling status page and project baseline | Merged; CI Gate passed |
| M1 | Bootloader target, App relocation, VTOR, and flash/debug flow | Complete; combined and App-only OpenOCD programming accepted on hardware |
| M2 | Signed image manifest, trailer, and host packaging | Software complete; key rotation requires Bootloader reflash; negative-path board acceptance pending |
| M3 | Assertions, reset capsule, logs, memory budgets, Diagnostic build | Planned |
| M4 | Pure-C core, input contracts, and host tests | Planned |
| M5 | Input hardware and normalized gesture/button events | Planned |
| M6 | LVGL 9.5 port, DMA flush, UI task, and 240x280 budget gate | Planned |
| M7 | XML generation and PC simulator using generated C | Planned |
| M8 | Page lifecycle and watch pages | Planned |
| M9 | Time, service queues, task health, and initialization policy | Planned |
| M10 | Confirmed sensor drivers and sensor service | Planned |
| M11 | Power states, wake sources, Stop recovery, and watchdog | Planned |
| M12 | W25Q128 raw driver, littlefs, and resource streaming | Planned |
| M13 | USB CDC resource protocol and KT6368 SPP transport | Planned |
| M14 | Candidate download, install recovery, trial boot, and rollback | Planned |
| M15 | Full Debug/Release/Diagnostic, simulator, fault injection, and final report | Planned |

## Current round

This round defines a fixed-offset signed application package. The 448 KiB
package contains the relocated App at offset zero, erased padding, and a 4 KiB
trailer at `0x0807F000`. Its 128-byte header carries the `F411` board id,
version, security counter, load address, image length, SHA-256, key id, and a
64-byte raw `r || s` ECDSA P-256 signature. The host packer uses OpenSSL; the
Bootloader verifies the same fields with the vendored TinyCrypt v0.2.8 subset.

Key id `0` was rotated after the original external private key could not be
recovered. The new P-256 public key is committed in the Bootloader; its private
key remains outside the repository. A board running the previous Bootloader
must be reflashed before the new package can be accepted.

OpenOCD/ST-Link is the supported F411 programming path. The VS Code tasks first
build and sign the package, then program the Bootloader and/or package. The
CubeProgrammer tasks and helper have been removed because they were unreliable
on this board.

Before key rotation, a valid package was verified on the host, programmed with
OpenOCD, and observed jumping to the App at `0x0801xxxx` after reset. M1
hardware acceptance also passed: combined programming, cold-start color bars,
App entry, interrupt response, and App-only programming with the Bootloader
region unchanged. After rotation, the new package is host-verified; remaining
M2 acceptance is to reflash the new Bootloader, confirm a valid package boots,
and reject an unsigned, corrupted, wrong-board, or out-of-range package.

## Next round

M3 will add assertions, reset capsules, diagnostic logs, and explicit memory
budgets for Debug, Release, and Diagnostic builds.

## Risks and blockers

- The LVGL Pro CLI is a local licensed generation tool. Generated C remains
  buildable without a CI Pro token, but regeneration requires a valid local
  license and must never place its token in Git.
- The security counter is signed in M2 but is not persisted or compared against
  confirmed metadata until the later OTA rounds.
- EEPROM type/address must be confirmed from the actual board before a driver
  is added; the old 24LC32 behavior is not evidence.
- KT6368 SPP firmware behavior and enable polarity require a board test; no
  undocumented AT command is assumed.
- There is no battery measurement baseline, so power acceptance is behavioral
  rather than a fabricated current target.

## Latest verification

For M1, the following passed from the F411 project directory:

```text
cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --target format-check
cmake --build --preset Debug --target cppcheck
```

For M2, `python tools/manifest/test_manifest.py` passed all eight host tests,
including package-padding rejection. The new key pair was generated outside
the repository, and the public half was installed in the Bootloader. The
generated package is exactly `458,752` bytes, with the trailer at package offset
`0x6F000`. The next required result is the manual Bootloader reflash,
valid-package cold start, and negative-path rejection listed above.
