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
| LVGL | Pinned v9.5.0; only the UI task calls LVGL, with a 240x20 double partial buffer |
| Display flush | ST7789 SPI1 DMA; RGB565 byte order is converted before the transfer and completion is acknowledged in the UI task |
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

- Reference commit: `d38e169` (M6 LVGL port and board acceptance merged to
  `main`).
- M0 through M6 CI Gates passed; M3a, M5, and M6 board acceptance are complete.
- Before relocation, the verified Debug App image was Flash `64,340 / 524,288 B`
  and RAM `30,208 / 131,072 B`.
- M3a pre-CDC Debug build: Bootloader was `6,564 B` Flash and `1,056 B` RAM;
  App was `65,280 B` Flash and `30,296 B` RAM.
- Current M6 Debug App is `234,772 B` Flash and `82,720 B` RAM; the Diagnostic
  App is `243,716 B` Flash and `82,720 B` RAM.
- M2 host packaging and OpenOCD programming remain the accepted upgrade path;
  CubeProgrammer is not part of the workflow.
- Current hardware facts: STM32F411, 24 MHz HSE, ST-Link-compatible SWD,
  ST7789 display, CST816 touch, W25Q128, and KT6368 UART wiring.

## Milestones

| ID | Scope | Status |
| --- | --- | --- |
| M0 | Rolling status page and project baseline | Merged; CI Gate passed |
| M1 | Bootloader target, App relocation, VTOR, and flash/debug flow | Complete; combined and App-only OpenOCD programming accepted on hardware |
| M2 | Signed image manifest, trailer, and host packaging | Software complete; key rotation requires Bootloader reflash; negative-path board acceptance pending |
| M3a | Assertions, reset capsule, memory budgets, Diagnostic build | Complete; Diagnostic cold-start and HardFault injection accepted on hardware |
| M3b | USB CDC logging and diagnostic transport | Complete locally; CubeMX CDC generation, software checks, OpenOCD programming, and COM6 host acceptance passed |
| M4 | Pure-C core, input contracts, and host tests | Complete; F411 and host CTest passed |
| M5 | Input hardware and normalized gesture/button events | Complete; M5a/M5b CI Gates and board acceptance passed |
| M6 | LVGL 9.5 port, DMA flush, UI task, and 240x280 budget gate | Complete; CI Gate and focused board acceptance passed |
| M7 | XML generation and PC simulator using generated C | Preflight blocked; local licensed LVGL Pro CLI is required |
| M8 | Page lifecycle and watch pages | Planned |
| M9 | Time, service queues, task health, and initialization policy | Planned |
| M10 | Confirmed sensor drivers and sensor service | Planned |
| M11 | Power states, wake sources, Stop recovery, and watchdog | Planned |
| M12 | W25Q128 raw driver, littlefs, and resource streaming | Planned |
| M13 | USB CDC resource protocol and KT6368 SPP transport | Planned |
| M14 | Candidate download, install recovery, trial boot, and rollback | Planned |
| M15 | Full Debug/Release/Diagnostic, simulator, fault injection, and final report | Planned |

## Current round

M6 pins upstream LVGL `v9.5.0` at commit
`85aa60d18b3d5e5588d7b247abf90198f07c8a63` under the F411 `third_party`
boundary. A hand-written `lv_conf.h` enables RGB565 software rendering, the
single label widget used by the representative page, a bounded 16 KiB LVGL
pool, and no demos, decoders, or board-specific LVGL driver.

The UI task is the only LVGL and `watch_core` owner. It creates the 240x280
display, uses two 240x20 partial draw buffers, converts LVGL's native RGB565
byte order into the ST7789 wire order, starts SPI1 DMA through the existing LCD
adapter, and calls `lv_display_flush_ready()` only after the transfer has been
observed complete or failed. The task also advances the LVGL tick and runs the
timer handler. The first page shows `MAGIC WATCH`, the current core page, and a
context hint; encoder/select events can move from the watchface to the launcher
and its two representative pages. USB CDC remains the diagnostic command and
log path, and no XML, simulator, or full page stack was added. The focused board
acceptance then confirmed the `MAGIC WATCH` page at 240x280, encoder/select and
screen-click navigation from `WATCHFACE` to `LAUNCHER` to `STATUS`, and a stable
left-edge right-swipe `BACK` gesture.

M7 preflight found no LVGL Pro CLI, XML source, generated UI C, simulator
project, or local license configuration in the repository or current environment.
No substitute generator or placeholder module is being added. The implementation
can start when the licensed CLI is made available through local environment
configuration; its token and absolute installation path must remain outside Git.

## Next round

The next implementation round remains M7: add the LVGL Pro CLI/XML generation
boundary and the PC simulator using committed generated C. M7 is blocked until
the local licensed CLI and its generation schema are available; M6 does not add
XML, generated UI, or simulator code.

## Risks and blockers

- The LVGL Pro CLI is a local licensed generation tool. Generated C remains
  buildable without a CI Pro token, but regeneration requires a valid local
  license and must never place its token in Git.
- The security counter is signed in M2 but is not persisted or compared against
  confirmed metadata until the later OTA rounds.
- EEPROM type/address must be confirmed from the actual board before a driver
  is added; the old 24LC32 behavior is not evidence.
- CST816 wiring, `0x15` address, the absence of `TP_INT`, encoder direction,
  and the button polarity are confirmed by the V2.1 reference project and the
  completed M5b board acceptance.
- KT6368 SPP firmware behavior and enable polarity require a board test; no
  undocumented AT command is assumed.
- There is no battery measurement baseline, so power acceptance is behavioral
  rather than a fabricated current target.
- The reset capsule survives software reset but not a complete power loss; a
  power-cycle fault-recovery claim is deferred until backup storage exists.
- M7 XML regeneration requires a local LVGL Pro CLI license and a documented
  project schema; CI must build committed generated C without a Pro token.

## Latest verification

For M6, the following passed from the F411 project directory and repository
root:

```text
cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --target format-check
cmake --build --preset Debug --target cppcheck
cmake --preset Diagnostic
cmake --build --preset Diagnostic
cmake -S tests -B build/host-tests-m6 -G Ninja
cmake --build build/host-tests-m6
ctest --test-dir build/host-tests-m6 --output-on-failure
```

M5b and M6 board acceptance are complete. M6 software validation passed `git diff
--check`, format-check, Cppcheck, the Debug/Diagnostic link-time budget checks,
the Debug/Diagnostic builds, and the host `watch_core_input` CTest. The linked
Debug App is Flash `234,772 B` and RAM `82,720 B`; Diagnostic is Flash `243,716 B`
and RAM `82,720 B`. The focused M6 board check is to flash the Debug image with
OpenOCD, confirm the 240x280 representative page is rendered without color-bar
fallback, then use encoder select/up/down to move between `WATCHFACE`,
`LAUNCHER`, `STATUS`, and `SETTINGS`. The USB stream should continue to show
the existing `input event=...` lines without unexpected `drop` or `i2c_err`
increments. The board result confirmed the representative page, encoder and
screen-click navigation, and left-edge `BACK`; it demonstrates the SPI1 DMA
flush and the single UI task, but does not accept any XML or simulator behavior.
