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

- This M9a task-ownership update began from clean `origin/main` at `55caa80`
  (`feat:f411:close diagnostic popup lifecycle`).
- The original pre-relocation Debug App reference was Flash `64,340 B` and RAM
  `30,208 B`. The current practical App budget remains 400 KiB Flash and 128 KiB
  RAM.
- Nine of 21 numbered milestones are fully closed: M0-M8. M9a is now closed
  as a sub-round; M9 remains partial pending the separate RTC/time contract.
  This remains about 43% by strict closed-loop scope and about 55% by
  implemented capability.
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
| M9 | RTC/time service, queues, heartbeats, init policy | Partial; M9a closes bootstrap task ownership, UI-inbound queue ownership, and board acceptance. The RTC/time contract remains M9b. |
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

## Current round: M9b time service

Add a pure-C local-calendar time contract, RTC adapter, normalized time events,
core snapshot data, watchface consumption, and CDC diagnostics. Time zones, NTP,
and network synchronization remain out of scope.

## Next round: M10b LIS2MDL

Add the LIS2MDL driver and board service as its own hardware-verified PR. Its
availability and failures remain explicit diagnostic states; aggregation remains
the later M10f round.

## Risks and blockers

- The signed M2 `security_counter` is not yet persisted or compared with a
  confirmed value. Metadata, trial confirmation, and replay rejection begin in
  M17-M19.
- Editor regeneration remains a manual LVGL Pro Editor operation. CI builds the
  committed generated C and does not replace the generator.
- The UI task previously overflowed its 4 KiB stack during Diagnostic work. It
  now uses 6 KiB and passed the popup sequence plus M9a CDC stability checks;
  broader duration regression remains part of M20.
- M9b has no RTC/local-calendar contract or time consumer yet.
- CDC diagnostic output can drop while no host reader is attached. The accepted
  M8d interaction sequence used snapshot state, not a zero-drop assertion.
- LSM6DS3 is the only closed sensor; missing sensors remain diagnostic degraded
  states rather than accepted sensor support.
- The no-battery board cannot establish KEY_WAKE, physical power-latch polarity,
  external watchdog wiring, or measured current reduction.
- W25Q128, resource storage, KT6368 transport, OTA metadata, install recovery,
  and rollback are not implemented.

## Latest verification

- Debug, Diagnostic, and Release App builds passed. Debug format-check and
  Cppcheck passed; host CTest passed 5/5 and the 240x280 simulator smoke test
  passed 1/1.
- App sizes remain within the 400 KiB practical Flash budget: Debug Flash
  `256,960 B` / RAM `83,144 B`; Diagnostic Flash `266,344 B` / RAM `83,144 B`;
  Release Flash `139,160 B` / RAM `83,136 B`.
- A newly signed Diagnostic package passed host verification, was written and
  readback-verified only in the App slot, and left the Bootloader 64 KiB
  SHA-256 unchanged.
- After dynamic CDC re-enumeration, `ping`, `info`, `health`, `diag`, and
  `stats` were valid. Health was stage 3 with app, UI, USB, and sensor services
  healthy and the UI event queue empty; no diagnostic capsule appeared during
  two ten-second stability segments.
- SWD observed four runtime tasks after boot, proving `defaultTask` had exited.
  Physical M9a acceptance completed encoder `SELECT -> popup=1`, then two
  left-edge `BACK` gestures for `popup=0` and Watchface return.
