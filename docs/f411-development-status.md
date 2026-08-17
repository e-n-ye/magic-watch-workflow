# F411 Development Status

This is the rolling current-state page for the STM32F411 reference firmware.
Git commits and pull requests retain the historical record. The numbered scope
is defined in [f411-development-plan.md](f411-development-plan.md).

## End state

The target is a real, runnable, diagnosable, upgradable, and rollbackable F411
watch reference firmware with a 240x280 UI, external resources, and signed OTA.
It is a reference implementation, not a production-product claim. Development
order remains hardware facts, pure-C contracts and host tests, board integration,
UI/resources, power, then secure OTA. No empty placeholder modules are added.

## Frozen decisions

| Area | Decision |
| --- | --- |
| F411 display | ST7789 at 240x280; LilyGo 240x240 is an independent future profile |
| Core and UI | Pure-C `watch_core`; one UI task owns LVGL and the core; services use bounded queues |
| Pages and input | Maximum stack depth four; one page tree and popup; normalized touch, encoder, `BACK`, and `WAKE` events |
| UI generation | LVGL 9.5.0; XML is the formal source; F411 builds committed generated C with `LV_USE_XML=0` |
| Boot layout | Bootloader `0x08000000-0x0800FFFF`; App slot `0x08010000-0x0807FFFF` |
| Image security | SHA-256 and ECDSA P-256 with a fixed 4 KiB trailer; private keys remain external |
| Storage and transport | W25Q128 raw OTA partitions remain separate from littlefs; KT6368 is transparent USART1 SPP; USB has no MSC |
| Security boundary | Integrity and replay protection only; no claim against SWD, unlocked RDP, or physical flash tampering |

## Baseline and completion

- This M10c AHT20-compatible update began from clean `origin/main` at `4636a21`
  (`feat:f411:add LIS2MDL Sensor Hub service`).
- The original pre-relocation Debug App reference was Flash `64,340 B` and RAM
  `30,208 B`. The current practical App budget remains 400 KiB Flash and 128 KiB
  RAM.
- Ten of 21 numbered milestones are fully closed: M0-M9. M10b LIS2MDL and M10c
  AHT20-compatible driver/service rounds are closed, while the five-sensor M10
  aggregate is still partial. This is about 48% by strict numbered scope. The
  current artifact is a runnable, diagnosable vertical slice; upgrade,
  power-loss recovery, and rollback are not present.
- Current hardware facts include STM32F411, 24 MHz HSE, ST-Link-compatible SWD,
  ST7789, CST816, W25Q128, KT6368, and a schematic EEPROM candidate
  `BL24C02F-RRRC`. EEPROM board address and response remain unconfirmed.

## Milestones

| ID | Scope | Current status |
| --- | --- | --- |
| M0 | Rolling status page and baseline | Complete; README links this page and the page records current state only. |
| M1 | Bootloader, App relocation, VTOR, flash/debug flow | Complete. |
| M2 | Manifest, trailer, host packaging, rejection paths | Complete for the signed factory-package contract. Valid App recovery and four board rejection paths are accepted. Persistent replay comparison belongs to OTA metadata rounds. |
| M3 | Budgets, assertions, reset capsule, CDC logs, Diagnostic | Complete. |
| M4 | Pure-C core and host tests | Complete. |
| M5 | CST816, encoder, button, normalized input | Complete. |
| M6 | LVGL 9.5, DMA flush, UI task, 240x280 budget | Complete. |
| M7 | Editor XML, generated C, PC simulator | Complete for the approved manual Editor export workflow. |
| M8 | Six page categories, lifecycle, interaction, leak pressure | Complete; the core-driven Diagnostic popup, physical input path, page lifecycle, and simulator pressure coverage are accepted. |
| M9 | RTC/time service, queues, heartbeats, init policy | Complete; M9a closed bootstrap and queue ownership, and M9b added the local-calendar RTC contract, normalized events, core snapshots, Watchface and CDC consumers, plus reset-retention board evidence. |
| M10 | Five sensor drivers and aggregation | Partial; LSM6DS3, LIS2MDL, and AHT20-compatible driver/service paths are implemented. M10c board evidence confirms AHT20/21 protocol response and valid samples; aggregation remains for M10f. |
| M11 | EEPROM part/address facts | Not started; `BL24C02F-RRRC` is a schematic candidate only. |
| M12 | Power and watchdog | Partial; RTC Stop/wake, display-off, software-off, and IWDG have software and no-battery board evidence. KEY_WAKE, physical cutoff, watchdog wiring, and current remain open. |
| M13 | W25Q128 raw driver | Not started. |
| M14 | littlefs and resource streaming | Not started. |
| M15 | USB diagnostic/log/resource protocol | Not started; existing CDC diagnostics are not the atomic resource protocol. |
| M16 | KT6368 SPP transport | Not started. |
| M17 | YModem candidate download and package verification | Not started. |
| M18 | Backup, install, and power-loss recovery | Not started. |
| M19 | Trial confirmation and rollback | Not started. |
| M20 | Final configurations, simulator, fault injection, budgets, regression report | Not started. |

## Current round: M10c AHT20

M10c is complete as a pure-C AHT20-compatible command, CRC, decode, and service
path, direct I2C board adapter, CDC diagnostic consumer, and Host test. The
legacy board project identifies the physical part as AHT21; AHT20 versus AHT21
marking is not independently confirmed, so this round records the shared
protocol result without claiming an exact package identity. Sensor aggregation
remains the separate M10f round.

## Next round: M10d MAX30102

M10c AHT20-compatible is the current isolated hardware-verified round; MAX30102
 follows it in M10d.

## Risks and blockers

- The signed M2 `security_counter` is not yet persisted or compared with a
  confirmed value. Metadata, trial confirmation, and replay rejection begin in
  M17-M19.
- Editor regeneration remains a manual LVGL Pro Editor operation. CI builds the
  committed generated C and does not replace the generator.
- The UI task previously overflowed its 4 KiB stack during Diagnostic work. It
  now uses 6 KiB and passed the popup sequence plus M9a CDC stability checks;
  broader duration regression remains part of M20.
- M9b reads the local RTC and retains it across reset, but does not add a
  calendar-setting or synchronization surface. Off-power retention remains
  unproven on the no-battery board.
- CDC diagnostic output can drop while no host reader is attached. The M10b
  session had an active reader and reported zero drops; this is not a general
  no-reader guarantee.
- The LIS2MDL schematic route is through the LSM6DS3 auxiliary Sensor Hub.
  M10b software integration and explicit degradation are complete, but the
  board returned `id=0x00` with no sample, so magnetometer support is not
  accepted until the auxiliary-I2C hardware facts are resolved.
- The legacy board project calls the 0x38 device AHT21 while this plan calls it
  AHT20. The shared command protocol and valid measurements are confirmed, but
  the exact AHT20/AHT21 package marking is not.
- The no-battery board cannot establish KEY_WAKE, physical power-latch polarity,
  external watchdog wiring, or measured current reduction.
- W25Q128, resource storage, KT6368 transport, OTA metadata, install recovery,
  and rollback are not implemented.

## Latest verification

- Debug, Diagnostic, and Release App builds passed. Debug format-check and
  Cppcheck passed; Host CTest passed `8/8` and the 240x280 simulator smoke test
  passed `1/1`.
- Current App budgets remain within the 400 KiB practical Flash budget:
  Debug Flash `269,328 B` / RAM `83,504 B`; Diagnostic Flash `278,744 B` /
  RAM `83,504 B`; Release Flash `145,016 B` / RAM `83,496 B`. Bootloader is
  `6,564 B` Flash / `1,056 B` RAM.
- M10b host tests cover Sensor Hub bank selection, LIS2MDL address/configuration,
  `sensor_hub_end_op` timeout, NACK handling, identity, sample decoding,
  periodic service behavior, and event-drop accounting.
- M10c Host tests cover AHT20/21 CRC validation, fixed-point decode, initialize
  and trigger command order, busy timeout, read failure, and event-drop
  accounting. A signed Diagnostic version 8/counter 8 package passed host
  verification and was written and verified at `0x08010000`. The 64 KiB
  Bootloader SHA-256 after the App-slot operation is
  `59034D2B990BFA044A1A928551E17828365D99ACB950D33F7E68371C9107FEB1`.
- After dynamic CDC re-enumeration, `ping`, `info`, `health`, `stats`, `diag`,
  `sensor`, and `power` were valid. Health was stage 3 with app, UI, USB, and
  sensor services healthy; the queue and CDC drop counters were zero and no
  diagnostic capsule was present.
- Board sensor evidence: LSM6DS3 was ready with ID `0x6A`, `5,394` samples,
  and zero read/event drops. LIS2MDL remained `ready=0`, `id=0x00`, sample
  count `0`, read errors `1`, NACK count `0`, and state `7`; this is recorded as
  the authorized Sensor Hub/auxiliary-I2C hardware risk, not as a passing
  magnetometer acceptance.
- Board AHT20/21 evidence: the direct `0x38` path reported `ready=1`,
  `cal=1`, valid samples with counts increasing from `131` to `304`, and zero
  read errors, CRC errors, busy timeouts, or event drops. Representative
  readings were `33.76 C` and `66.26%` relative humidity; status returned
  `0x18` when ready. The dynamic CDC reader also confirmed health stage 3,
  zero queue/CDC drops, `diag=none`, and normal power/watchdog state.
