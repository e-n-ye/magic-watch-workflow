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

- Reference commit: `dd1dcd2` (M3a Diagnostic hardware acceptance merged to
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
| M4 | Pure-C core, input contracts, and host tests | Complete; F411 and host CTest passed |
| M5 | Input hardware and normalized gesture/button events | In progress; M5a pure-C normalization complete, M5b board loop pending |
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

M5a adds the pure C `watch_input` normalizer under `products/f411_watch/input`.
It debounces BACK, WAKE, and encoder-button samples; maps encoder deltas to
single-step UP/DOWN events; maps taps and vertical swipes; and accepts a right
swipe only when it starts within the frozen 240-pixel display edge. Output is a
bounded `watch_event_t` queue that can be consumed by `watch_core` without any
HAL, FreeRTOS, or LVGL dependency.

The F411 target compiles the shared input module and host CTest covers debounce,
queue-full atomicity, touch gesture mapping, and core dispatch. M5a does not
claim a board loop: CST816 software I2C, GPIO/EXTI callback ownership, TIM4
encoder startup, LCD/USB feedback, and hardware polarity remain in M5b.

## Next round

The next implementation sub-round is M5b: confirm the CST816 bus/address and
touch wiring, start/consume TIM4 encoder counts, route GPIO EXTI callbacks, and
exercise the normalized events through USB/LCD diagnostics on hardware. The
separate M3a Diagnostic cold-start and deliberate fault-injection checks remain
accepted manual items.

## Risks and blockers

- The LVGL Pro CLI is a local licensed generation tool. Generated C remains
  buildable without a CI Pro token, but regeneration requires a valid local
  license and must never place its token in Git.
- The security counter is signed in M2 but is not persisted or compared against
  confirmed metadata until the later OTA rounds.
- EEPROM type/address must be confirmed from the actual board before a driver
  is added; the old 24LC32 behavior is not evidence.
- CST816 has TP_RST/TP_SDA/TP_SCL evidence but no confirmed TP_INT or I2C
  address in the current project; M5b must not inherit the old driver's
  software-I2C assumptions without board verification.
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

For M5a, the following passed from the F411 project directory and repository
root:

```text
cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --target format-check
cmake --build --preset Debug --target cppcheck
cmake --preset Diagnostic
cmake --build --preset Diagnostic
cmake -S tests -B build/host-tests -G Ninja
cmake --build build/host-tests
ctest --test-dir build/host-tests --output-on-failure
```

M5a software validation passed `git diff --check`, format-check, Cppcheck, the
Debug/Diagnostic link-time budget checks, the Debug/Diagnostic builds, and the
host `watch_core_input` CTest. No board acceptance was performed; M5b is the
next manual hardware boundary.
