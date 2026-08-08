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

- Reference commit: `5346cf7` (M3b USB CDC diagnostic transport merged to
  `main`).
- M0, M1, M2, M3a, and M3b CI Gates passed; M3a board acceptance is complete.
- Before relocation, the verified Debug App image was Flash `64,340 / 524,288 B`
  and RAM `30,208 / 131,072 B`.
- M3a pre-CDC Debug build: Bootloader was `6,564 B` Flash and `1,056 B` RAM;
  App was `65,280 B` Flash and `30,296 B` RAM.
- Current M3b Debug App is `65,972 B` Flash and `36,936 B` RAM; the Diagnostic
  App is `74,596 B` Flash and `36,936 B` RAM.
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

M3a adds a reset-surviving diagnostic capsule for HardFault, other Cortex-M
faults, HAL errors, full assertions, and FreeRTOS stack overflow. The capsule
stores core fault registers and a checksum in a linker-defined `.noinit`
section. On the next software reset, the App shows a reason-coded LCD pattern
once and then clears the capsule; a normal boot keeps the existing 240x280
color bars.

Debug and Diagnostic builds now enforce the App `400 KiB` Flash budget and
`128 KiB` RAM budget after linking. The standalone Bootloader enforces its `64
KiB` Flash boundary and the same RAM boundary. Diagnostic enables
`USE_FULL_ASSERT=1`; Debug keeps the existing assertion behavior.

M3b changes the CubeMX USB device class to CDC while keeping PA11/PA12, the
48 MHz USB clock, and USART1 for the KT6368 link. A hand-written board transport
owns bounded RX/TX rings and a non-blocking 64-byte packet sender. The service
task accepts line-oriented `help`, `ping`, `info`, `diag`, and `stats` commands;
unknown commands and overlong lines return explicit errors. No `printf`
redirection, MSC mode, resource protocol, or OTA transport is included here.

M3a board acceptance is now complete. A Diagnostic App cold start after a full
power cycle showed the backlight and the normal 240x280 color bars. A controlled
ST-Link/OpenOCD/GDB fault injection then stopped the CPU in the diagnostic halt
path; a reset showed the one-time HardFault pattern (observed as roughly 30%
black and 70% yellow), and the following reset restored the normal color bars.
The capsule is only required to survive a reset, not a complete power loss.

The generated HardFault function remains a CubeMX placeholder. A source-level
CMake symbol remap routes the vector table to the hand-written diagnostic
handler under `user/app`, so a later CubeMX regeneration does not overwrite the
M3a fault capture path.

## Next round

The next implementation round is M4: the pure-C `watch_core` state machine,
normalized events, snapshots, commands, and host tests. The separate M3a
Diagnostic cold-start and deliberate fault-injection checks remain manual
acceptance items.

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
- The reset capsule survives software reset but not a complete power loss; a
  power-cycle fault-recovery claim is deferred until backup storage exists.
- USB CDC enumeration, bidirectional command exchange, reconnect behavior, and
  ring-buffer overflow counters require a manual host/board check; CI has no USB
  hardware job.

## Latest verification

For M3b, the following passed from the F411 project directory:

```text
cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --target format-check
cmake --build --preset Debug --target cppcheck
cmake --preset Diagnostic
cmake --build --preset Diagnostic
```

M3b software validation passed `git diff --check`, format-check, Cppcheck, both
Debug/Diagnostic link-time budget checks, and the Debug/Diagnostic builds. The
combined OpenOCD task wrote and verified the Bootloader plus signed App. The
board enumerated as `USB Serial Device (COM6)` with VID/PID `0483:5740`; `ping`,
`info`, `diag`, `stats`, and `help` returned the expected lines, an unknown
command and an overlong line returned explicit errors, and a close/reopen cycle
accepted `ping` again. M3a Diagnostic cold-start and deliberate HardFault
injection were then accepted on hardware using the Diagnostic App, OpenOCD, and
GDB; the capsule pattern and post-reset recovery matched the acceptance steps.
