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

- This M9b time-service update began from clean `origin/main` at `e91b4a9`
  (`feat:f411:close M9a task ownership`).
- The original pre-relocation Debug App reference was Flash `64,340 B` and RAM
  `30,208 B`. The current practical App budget remains 400 KiB Flash and 128 KiB
  RAM.
- Ten of 21 numbered milestones are fully closed: M0-M9. This is about 48% by
  strict closed-loop scope. The current artifact is a runnable, diagnosable
  vertical slice; upgrade, power-loss recovery, and rollback are not present.
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
| M10 | Five sensor drivers and aggregation | Partial; only LSM6DS3 has a closed driver/service path. |
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

## Current round: M10b LIS2MDL

The next isolated hardware closure is the LIS2MDL driver and board service.
Availability and failures remain explicit diagnostic states; sensor aggregation
remains the separate M10f round.

## Next round: M10c AHT20

The AHT20 driver and board service remain a separate hardware-verified PR after
M10b.

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
- During M9b App-slot recovery, unused bytes in the reserved Bootloader area
  changed. The compiled Bootloader range matched before and after, but no full
  64 KiB raw-preservation assertion is claimed.
- CDC diagnostic output can drop while no host reader is attached. The board
  session reported transmit drops, so acceptance does not claim a zero counter.
- LSM6DS3 is the only closed sensor; missing sensors remain diagnostic degraded
  states rather than accepted sensor support.
- The no-battery board cannot establish KEY_WAKE, physical power-latch polarity,
  external watchdog wiring, or measured current reduction.
- W25Q128, resource storage, KT6368 transport, OTA metadata, install recovery,
  and rollback are not implemented.

## Latest verification

- Debug, Diagnostic, and Release App builds passed. Debug format-check and
  Cppcheck passed; host CTest passed 6/6 and the 240x280 simulator smoke test
  passed 1/1, including Watchface time rendering without page recreation.
- Current App ELF budgets remain within the 400 KiB practical Flash budget:
  Debug Flash `260,156 B` / RAM `83,376 B`; Diagnostic Flash `269,572 B` /
  RAM `83,376 B`; Release Flash `140,804 B` / RAM `83,360 B`.
- A signed Diagnostic package passed host verification and App-slot write/read
  verification. After dynamic CDC re-enumeration, `ping`, `info`, `health`,
  `diag`, and `stats` were valid; health was stage 3 with app, UI, USB, and
  sensor services healthy and the UI event queue empty.
- Reset-only RTC retention preserved the local calendar. Two successive CDC
  `info` responses advanced by two seconds on the Watchface.
- Physical M9b acceptance completed Diagnostics `BACK` to Watchface, observed
  central `HH:MM:SS` progression, then completed encoder `SELECT` to Launcher
  and `BACK` to Watchface. No diagnostic capsule appeared.
