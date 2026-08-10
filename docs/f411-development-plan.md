# F411 Reference Firmware Development Plan

This document is the source of truth for the numbered F411 implementation
rounds. The status page records the current evidence; Git history and pull
requests retain the detailed process history.

## Goal and order

The end product is a real, runnable, diagnosable, upgradable, and rollbackable
STM32F411 watch reference firmware. It is a reference implementation, not a
claim of a production product.

The fixed order is:

1. Verify hardware facts.
2. Define pure-C contracts and host tests.
3. Close the board-level vertical loop.
4. Add UI and resources.
5. Add power behavior.
6. Add secure OTA.

F411 uses a frozen 240x280 display profile. LilyGo 240x240 is a future,
independent board profile and does not affect F411 XML, simulator, or acceptance
dimensions.

Each numbered round is one focused branch, one themed commit, and one pull
request. After merge, the next round starts from the latest `origin/main`. Every
feature pull request updates the rolling status page.

## Frozen architecture

- `watch_core` is a pure-C state machine with no HAL, FreeRTOS, or LVGL. Its
  contract is initialization, dispatching `watch_event_t`, reading
  `watch_snapshot_t`, and taking `watch_command_t` values.
- `ui_task` owns LVGL and `watch_core`. Input, sensors, storage, and connection
  services submit events through bounded queues and receive commands through a
  command queue.
- The page stack is at most four levels deep. Only one page object tree and one
  popup exist at a time; leaving a page destroys its controls while business
  state remains in the core.
- The page set is watch face, fixed-list launcher, status/sensors, settings,
  resource viewer, and hidden diagnostics. Touch supports click and left-edge
  swipe-back; scrolling is used only for overflowing content. Encoder rotation,
  press, `BACK`, and `WAKE` map to the same normalized input events.
- `defaultTask` only starts runtime tasks and then exits. It does not own watch
  business logic. Board drivers expose device-semantic APIs, not HAL handles,
  GPIO/SPI/I2C details, or registers.

## Boot and security contract

- Internal flash layout is Bootloader `0x08000000-0x0800FFFF` and Application
  slot `0x08010000-0x0807FFFF`.
- The application slot is 448 KiB. Its final 4 KiB is a signature trailer, so
  the link region is at most 444 KiB; CI keeps the practical App budget at
  400 KiB.
- Bootloader is a separate bare-metal CMake target using only startup/CMSIS,
  required HAL, W25, and UART adapters. It must not include RTOS, LVGL, or
  littlefs.
- Images use SHA-256 and ECDSA P-256 with a fixed 64-byte `r || s` signature.
  The target links a pinned TinyCrypt verification subset and records its
  license. The private key is read by the host packager from an external file;
  it never enters the repository, firmware, or CI logs. The Bootloader embeds
  only the public key.
- The manifest is a fixed-offset little-endian binary format containing format
  version, board, firmware version, security counter, load address, image
  length, SHA-256, public-key id, and signature. Packed structs are not mapped
  directly during parsing.
- OTA rejects a wrong board, out-of-range image, bad signature, or a security
  counter below the last confirmed value. The security boundary covers remote
  image integrity and replay prevention only; it does not claim resistance to
  SWD, unlocked RDP, or physical external-flash modification.

## W25Q128 and OTA contract

Raw W25Q128 partitions are fixed as metadata `0x000000-0x00FFFF`, candidate
`0x010000-0x08FFFF`, rollback `0x090000-0x10FFFF`, and littlefs
`0x110000-0xFFFFFF`. The Bootloader never mounts littlefs and accesses only the
first three partitions. Metadata uses two 4 KiB sectors as an increasing-sequence
CRC32 double-copy log.

OTA states are downloading, candidate-ready, backing-up, installing, trial,
confirmed, pending-rollback, rolling-back, and error. Before installation the
current signed App is backed up in full. Installation and rollback steps are
idempotent and resume from the last valid state after power loss.

Before jumping, the Bootloader increments the trial count. The App confirms
after 30 seconds of healthy UI, input, supervisor, watchdog, and W25 metadata
operation. At most three unconfirmed boots are allowed; the fourth restores
rollback. A missing sensor is a degraded state and does not prevent
confirmation.

KT6368 is only a USART1 `115200 8N1` SPP transparent transport. F411 uses
DMA+IDLE ring buffering and YModem; the host sends the same signed package over
Bluetooth serial or USB CDC. No phone app, image encryption, or undocumented AT
command is part of this plan.

## Numbered rounds

| Round | Closed loop and acceptance result |
| --- | --- |
| M0 | Create the rolling status page, link it from README, and record the frozen end state, baseline, current/next round, risks, and latest verification. |
| M1 | Create the minimum Bootloader, relocate the App to `0x08010000`, fix VTOR/linker/programming/debug flows, and retain the existing color-bar cold-start acceptance. |
| M2 | Define the signed manifest, 4 KiB trailer, and host packager. Bootloader accepts a valid factory image and rejects corruption, overflow, and wrong-board images. |
| M3 | Add map/size budgets, static and runtime assertions, HardFault/reset capsule, USB CDC ring logging, and Diagnostic presets. |
| M4 | Implement pure-C core, navigation state machine, snapshot/command interfaces, and host unit tests without UI. |
| M5 | Close CST816, encoder, and button hardware behavior using logs and the existing LCD diagnostic path; verify click, swipe-back, debounce, and fast ISR exit. |
| M6 | Pin LVGL 9.5.0; implement ST7789 DMA flush, tick, one UI task, and a representative 240x280 page; continue only after resource budgets pass. |
| M7 | Integrate LVGL Pro Editor XML and the PC simulator. XML is the formal source and generated C is committed. F411 keeps `LV_USE_XML=0`. The approved workflow is manual Editor Code/export; no Pro CLI, token, or local absolute path is required. |
| M8 | Implement page lifetime, all six page categories, normalized-input-to-core-to-UI interaction, and repeated enter/leave leak tests. |
| M9 | Implement RTC/time service, bounded event queues, task heartbeats, and centralized initialization order. |
| M10 | Deliver one focused PR per LSM6DS3, LIS2MDL, AHT20, MAX30102, and CW2015, then add the sensor aggregation service. |
| M11 | Verify the EEPROM part number and address. If facts remain unclear, record the risk only; do not migrate the legacy 24LC32 driver. |
| M12 | Implement display-off, WAKE/RTC wake, Stop recovery, software shutdown, and independent watchdog. There is no battery, so do not publish a fabricated current target. |
| M13 | Implement a reusable W25Q128 protocol driver and verify JEDEC ID, page writes, 4 KiB erase, timeouts, and error recovery. |
| M14 | Freeze partitions, mount littlefs, and prove chunked reads for images, fonts, and long text while keeping OTA partitions outside the filesystem. |
| M15 | Extend USB CDC with diagnostics, logs, and atomic resource transfer. Do not implement online MSC. |
| M16 | Verify KT6368 enable polarity, SPP pairing, sustained DMA+IDLE transfer, and recovery from link faults without assuming undocumented commands. |
| M17 | Download by YModem into candidate with per-packet CRC16, then verify manifest, SHA-256, signature, and security counter; failures must not touch the internal App. |
| M18 | Implement backup, installation, and power-loss recovery; inject faults at selected erase/write boundaries in the host model and on hardware. |
| M19 | Implement trial confirmation, three-failure rollback, watchdog/HardFault rollback, and diagnostic error states. |
| M20 | Complete Debug/Release/Diagnostic, host tests, simulator, resource budgets, and full-device regression, then publish the final reference-firmware acceptance report. |

## Per-round validation

From M1 onward, every F411 pull request runs from
`firmware/stm32/f411_watch`:

```sh
cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --target format-check
cmake --build --preset Debug --target cppcheck
```

The relevant round additionally runs host `ctest`, simulator tests, ELF/map
checks, and the stated cold-start, power-loss, or board acceptance. Results are
written to the rolling status page. The documentation-only M0 correction may
skip the F411 build, but its pull request must still finish with a successful
`CI / CI Gate` and use Rebase and merge.

## XML and simulator rules

The local Editor is discovered through environment configuration. No
`D:\\...` installation path or Pro token is committed. CI compiles committed
generated output and validates the XML, Editor/LVGL version, and input/output
summary; it does not require a long-lived Pro license. If the trial license is
unavailable, the existing firmware can still build, but regeneration is
explicitly blocked rather than silently switching generators.

The simulator reuses `watch_core`, generated UI, page wrappers, and gesture
recognition. It does not simulate HAL or FreeRTOS. Its F411 acceptance profile
is only 240x280. LilyGo 240x240 requires a separate board profile and export
batch.
