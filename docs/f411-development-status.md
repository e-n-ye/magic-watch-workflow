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
- Current M5b Debug App is `72,324 B` Flash and `37,112 B` RAM; the Diagnostic
  App is `81,188 B` Flash and `37,112 B` RAM.
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
| M5 | Input hardware and normalized gesture/button events | M5a merged; M5b software loop complete, board acceptance pending |
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

M5b adds the F411 board loop on top of the M5a `watch_input` normalizer. The
board facts are now confirmed from the V2.1 reference project: CST816 address
`0x15`, software I2C on `PA3=SDA` and `PA4=SCL`, reset on `PA2`, no `TP_INT`,
BACK=`PB10` active-low, WAKE=`PB12` active-high, encoder key=`PB2`
active-low, and TIM4 encoder `PB6=ENCODER_B/CH1`, `PB7=ENCODER_A/CH2`.

The hand-written board adapter polls CST816 gestures, starts and filters TIM4
counts, samples the three buttons through the normalized debounce contract, and
keeps EXTI callbacks short by recording counters only. The board adapter now
reverses the TIM4 sign to match the observed physical encoder direction;
the pure-C input contract and host tests remain unchanged. One non-empty CST816
gesture is held until a `gesture=0` sample so a single swipe cannot emit several
directions, and `SLIDE_LEFT` remains explicitly ignored. USB CDC emits the
input status/event lines plus raw touch gesture, finger count, coordinates,
normalized mapping, and queue result. The LCD remains color-bar-only; text
feedback is deferred to the LVGL round. No CubeMX-generated file, `.ioc`, LVGL,
XML, or real page was added.

## Next round

The next implementation round is M6: fix the LVGL 9.5 integration boundary,
ST7789 DMA flush, tick ownership, and the first 240x280 representative page.
M5b still requires a focused board acceptance before it can be marked complete;
the separate M3a Diagnostic cold-start and deliberate fault-injection checks
remain accepted manual items.

## Risks and blockers

- The LVGL Pro CLI is a local licensed generation tool. Generated C remains
  buildable without a CI Pro token, but regeneration requires a valid local
  license and must never place its token in Git.
- The security counter is signed in M2 but is not persisted or compared against
  confirmed metadata until the later OTA rounds.
- EEPROM type/address must be confirmed from the actual board before a driver
  is added; the old 24LC32 behavior is not evidence.
- CST816 wiring, `0x15` address, and the absence of `TP_INT` are now confirmed
  from the V2.1 reference project; M5b still needs the board transaction and
  gesture check before this software fact becomes a hardware acceptance.
- KT6368 SPP firmware behavior and enable polarity require a board test; no
  undocumented AT command is assumed.
- There is no battery measurement baseline, so power acceptance is behavioral
  rather than a fabricated current target.
- The reset capsule survives software reset but not a complete power loss; a
  power-cycle fault-recovery claim is deferred until backup storage exists.
- M5b input status/event lines, button debounce, corrected encoder direction,
  touch gesture mapping, and LCD color feedback still require a focused manual
  host/board retest; CI has no input hardware job.

## Latest verification

For M5b, the following passed from the F411 project directory and repository
root:

```text
cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --target format-check
cmake --build --preset Debug --target cppcheck
cmake --preset Diagnostic
cmake --build --preset Diagnostic
cmake -S tests -B build/host-tests-m5b -G Ninja
cmake --build build/host-tests-m5b
ctest --test-dir build/host-tests-m5b --output-on-failure
```

M5b software validation passed `git diff --check`, format-check, Cppcheck, the
Debug/Diagnostic link-time budget checks, the Debug/Diagnostic builds, and the
host `watch_core_input` CTest. Board acceptance remains pending. After flashing
the revised Debug image with OpenOCD, verify the USB line beginning `input hw`
reports `dir=reverse`, then rotate the encoder in both directions and tap the
panel. Each accepted normalized event must produce one USB `input event=...`
line; each touch gesture must also produce an `input touch gesture=...` line
with raw coordinates, `map=...`, and `queued=...`. A right-edge left swipe must
show `map=none queued=0`, while a left-edge right swipe is the only gesture
expected to show `map=back`. The status line must show no unexpected `drop` or
`i2c_err` increments. BACK/WAKE and a final full M5b acceptance remain pending.
