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
| XML | XML is maintained in LVGL Pro Editor; generated C is produced only by Editor Code/export and committed; generated files are never hand-edited; F411 does not parse XML at runtime |
| Boot layout | Bootloader `0x08000000-0x0800FFFF`; application `0x08010000-0x0807FFFF` |
| App budget | 448 KiB slot with a 4 KiB signed trailer; linked image budget is 400 KiB |
| External flash | W25Q128 metadata, candidate, rollback, and littlefs partitions are separate |
| OTA security | SHA-256 plus ECDSA P-256 signature, board/version checks, trial confirmation, rollback |
| Bluetooth | KT6368 is a transparent SPP/UART transport; protocol and validation stay on F411/host |
| USB | CDC for diagnostics, logs, and controlled resource transfer; no online MSC |
| Security boundary | No claim of protection from SWD/RDP-disabled physical modification |

## Baseline

- Reference commit: `3b2dad8` (M7 Editor-exported UI and XML development skill
  merged to `main`).
- M0 through M6 CI Gates passed; M3a, M5, and M6 board acceptance are complete.
- Before relocation, the verified Debug App image was Flash `64,340 / 524,288 B`
  and RAM `30,208 / 131,072 B`.
- M3a pre-CDC Debug build: Bootloader was `6,564 B` Flash and `1,056 B` RAM;
  App was `65,280 B` Flash and `30,296 B` RAM.
- Current M6 Debug App was `234,772 B` Flash and `82,720 B` RAM; the Diagnostic
  App was `243,716 B` Flash and `82,720 B` RAM.
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
| M7 | Editor-exported XML UI, generated C, and PC simulator | Implemented locally; host CTest passed; no board acceptance required |
| M8 | Page lifecycle and watch pages | Functional board acceptance passed; exported screen opacity corrected and fixed-image board check passed |
| M9 | Time, service queues, task health, and initialization policy | USB health and fixed-image board acceptance passed |
| M10 | Confirmed sensor drivers and sensor service | Complete; LSM6DS3 host model, F411 polling service, and board USB acceptance passed |
| M11 | Power states, wake sources, Stop recovery, and watchdog | Implemented locally; host/F411 checks passed; board acceptance pending |
| M12 | W25Q128 raw driver, littlefs, and resource streaming | Planned |
| M13 | USB CDC resource protocol and KT6368 SPP transport | Planned |
| M14 | Candidate download, install recovery, trial boot, and rollback | Planned |
| M15 | Full Debug/Release/Diagnostic, simulator, fault injection, and final report | Planned |

## Current round

M11 adds a pure-C power state contract and watchdog contract, then connects the
F411 adapter to the existing LSE-backed RTC, KEY_WAKE EXTI, and PWR Stop entry.
The USB diagnostic consumer can request display-off, Stop, or a software-off
state. Stop suspends the HAL/RTOS tick and temporarily masks the USB IRQ;
the RTC wake timer and KEY_WAKE restore the PLL clock and resume the scheduler.
The independent watchdog uses the STM32 IWDG with a board adapter and is
refreshed only while APP, UI, USB, and sensor health records are all current.

The software-off command only blanks the display and enters the power contract's
`off` state; it intentionally keeps `POWER_EN` asserted until the board-level
power-latch polarity is separately confirmed. M11 does not add automatic idle
timeouts, sensor interrupts, new UI pages, battery current targets, or a claim
that the external WDI/WDOG_EN circuit has been validated.

## Next round

M12 will add the W25Q128 raw driver, littlefs partitioning, and resource
streaming. M11 remains limited to behavioral power transitions and does not
expand the sensor set or OTA/storage behavior.

## Risks and blockers

- Editor export is a manual, local generation step. CI builds committed generated
  C but does not regenerate it, so the Editor version and export contract remain
  part of the M7 record.
- The security counter is signed in M2 but is not persisted or compared against
  confirmed metadata until the later OTA rounds.
- EEPROM type/address must be confirmed from the actual board before a driver
  is added; the old 24LC32 behavior is not evidence.
- CST816 wiring, `0x15` address, the absence of `TP_INT`, encoder direction,
  and the button polarity are confirmed by the V2.1 reference project and the
  completed M5b board acceptance.
- LSM6DS3 wiring and `0x6A` address are taken from the V2.1 reference project:
  I2C1 is PB8/PB9, INT1 is PA8, and INT2 is PB15. M10 polls the device and does
  not claim the interrupt paths are validated.
- Stop acceptance depends on the board's LSE, RTC wake IRQ, KEY_WAKE EXTI, and
  USB re-enumeration behavior; the code does not claim a measured current
  reduction. The IWDG timeout uses the internal LSI's nominal frequency and is
  a reset-safety value, not a precision timebase.
- `POWER_EN` polarity and the external WDI/WDOG_EN circuit remain unverified;
  M11's software-off path therefore leaves the power latch asserted.
- KT6368 SPP firmware behavior and enable polarity require a board test; no
  undocumented AT command is assumed.
- There is no battery measurement baseline, so power acceptance is behavioral
  rather than a fabricated current target.
- The reset capsule survives software reset but not a complete power loss; a
  power-cycle fault-recovery claim is deferred until backup storage exists.
- M7 regeneration requires a local LVGL Pro Editor project and a documented
  export procedure; CI only builds the committed generated C and does not need
  a Pro token.

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

For M7, the following host validation also passed from the repository root:

```text
cmake -S products/f411_watch/simulator -B build/host-m7 -G Ninja
cmake --build build/host-m7
ctest --test-dir build/host-m7 --output-on-failure
build/host-m7/watch_ui_simulator.exe --smoke
```

The M7 smoke output was `watch_ui_smoke: PASS display=240x280 ui=MAGIC WATCH
core=LAUNCHER`. This is a host/UI acceptance only; no new board flashing or
manual M7 hardware demonstration is required. The follow-up preview check also
validated the Editor-compatible `<style name="..." />` view syntax, the XML
documents parse successfully, and the Windows native path waits for its delayed
framebuffer allocation before forcing the first visible frame.

For the current M8 Editor export integration, the following passed from the
repository root:

```text
cmake -S products/f411_watch/simulator -B build/host-m8-editor -G Ninja
cmake --build build/host-m8-editor
ctest --test-dir build/host-m8-editor --output-on-failure
build/host-m8-editor/watch_ui_simulator.exe --smoke
cmake -S tests -B build/host-tests-m8-editor -G Ninja
cmake --build build/host-tests-m8-editor
ctest --test-dir build/host-tests-m8-editor --output-on-failure
```

The lifecycle smoke output was `watch_ui_smoke: PASS display=240x280 pages=5
creates=5 destroys=4 active=WATCHFACE`. The four screen XML files, globals,
translations, and project metadata parse as XML. The F411 Debug checks also
passed:

```text
cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --target format-check
cmake --build --preset Debug --target cppcheck
```

The linked Debug App is Flash `242,356 B` and RAM `82,808 B`, under the 400 KiB
App and 128 KiB RAM limits. The Editor preview runtime remains local and is
ignored as `preview-bin`; only the exported C/H and build lists are candidates
for Git. M8 board acceptance passed: the Debug image showed `MAGIC WATCH` and
the four page labels on the 240x280 display, select/down reached `LAUNCHER`,
`STATUS`, and `SETTINGS`, and the existing left-edge `BACK` gesture returned to
the previous page. The USB log had the expected normalized events without
unexpected `drop` or `i2c_err` increments.

For M9, the following validation passed from the repository root and F411
project directory:

```text
cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --target format-check
cmake --build --preset Debug --target cppcheck
cmake -S tests -B build/host-tests-m9 -G Ninja
cmake --build build/host-tests-m9
ctest --test-dir build/host-tests-m9 --output-on-failure
cmake -S products/f411_watch/simulator -B build/host-m9 -G Ninja
cmake --build build/host-m9
ctest --test-dir build/host-m9 --output-on-failure
```

The host runtime suite passed the wrap-safe time, initialization order, bounded
queue, FIFO, and heartbeat timeout checks. The existing core/input suite and
the LVGL page lifecycle smoke also passed. The linked Debug App is Flash
`244,384 B` and RAM `82,968 B`, under the 400 KiB and 128 KiB limits.

M9 USB CDC health acceptance passed: `health\r\n` reported `stage=3`, `app=ok`,
`ui=ok`, `usb=ok`, and `queue=0`; `help\r\n` listed `health`. The corrected
Debug App was then flashed with OpenOCD and verified on the board; the 240x280
pages showed the XML dark background (`0x101820`) with the expected text
colors. No new sensor, RTC, watchdog, or power behavior is part of this
demonstration.

For M10, the following host and F411 checks passed on the LSM6DS3 branch:

```text
cmake -S tests -B build/host-tests-m10 -G Ninja
cmake --build build/host-tests-m10
ctest --test-dir build/host-tests-m10 --output-on-failure
cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --target format-check
cmake --build --preset Debug --target cppcheck
```

The new host suite passed the LSM6DS3 WHO_AM_I/configuration, raw sample
decoding, periodic service, and bounded event-drop checks. The linked Debug App
is Flash `250,220 B` and RAM `83,048 B`, under the 400 KiB and 128 KiB limits.
The signed Debug App was flashed with OpenOCD and the board USB CDC acceptance
passed: `health\r\n` reported the sensor service, and `sensor\r\n` reported
`lsm6ds3=1`, `id=0x6a`, a nonzero sample count, and zero read/event drops. The
existing UI remained usable; no interrupt-path or sensor-widget behavior is
claimed. A missing device still reports `lsm6ds3=0` without preventing the UI
from starting, but that is a diagnostic result rather than a passing sensor
acceptance.

For M11, the following host and F411 checks passed on the power branch:

```text
cmake -S tests -B build/host-tests-m11 -G Ninja
cmake --build build/host-tests-m11
ctest --test-dir build/host-tests-m11 --output-on-failure
cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --target format-check
cmake --build --preset Debug --target cppcheck
```

The host suite passed the power-state transition and health-gated watchdog
tests (`watch_power`), together with the existing core, runtime, and LSM6DS3
tests. The linked Debug App is Flash `254,304 B` and RAM `83,096 B`, under the
400 KiB and 128 KiB limits.

M11 board acceptance is still required. After flashing the signed Debug App
with OpenOCD, send `help\r\n` and confirm that `power`, `display-off`, `sleep`,
and `shutdown` are listed. Send `power\r\n` and confirm `state=active`,
`watchdog=1`, an increasing `refresh` count, and `blocked=0 fail=0`. Send
`display-off\r\n`; the backlight should turn off and `power` should report
`state=display-off`. Press KEY_WAKE and confirm the backlight returns and the
state becomes `active wake=key`. Send `sleep\r\n`; the display should remain
off while the MCU enters Stop, then the board should wake after roughly three
seconds through RTC (`wake=rtc`) or immediately through KEY_WAKE (`wake=key`).
After USB re-enumeration if necessary, `power` should report `stops=1 wakes=1`
and the existing UI should remain usable. Finally, `shutdown\r\n` should report
`state=off` with the backlight off; KEY_WAKE should return to `active`. This is
a software-off check only and must not be treated as proof of physical power
cutoff.
