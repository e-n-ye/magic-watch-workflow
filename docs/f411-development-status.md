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

- This M2 board-acceptance update began from clean `origin/main` at `a8fe734`
  (`feat:f411:add diagnostic runtime preset`).
- The original pre-relocation Debug App reference was Flash `64,340 B` and RAM
  `30,208 B`. The current practical App budget remains 400 KiB Flash and 128 KiB
  RAM.
- Eight of 21 numbered milestones are fully closed: M0-M7. This is about 38%
  by strict closed-loop scope and about 50% by implemented capability.
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
| M8 | Six page categories, lifecycle, interaction, leak pressure | Partial; page lifecycle, physical normal navigation, Diagnostic preset, and simulator pressure are present. No real core-driven popup consumer exists yet. |
| M9 | RTC/time service, queues, heartbeats, init policy | Partial; bounded queue and health foundations exist, but task ownership and the RTC/time contract are incomplete. |
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

## Current round: M8d diagnostic popup closure

Add a real Diagnostic-page popup state to the pure-C core snapshot and command
contract. `SELECT` opens the popup, the first `BACK` closes it, and the next
`BACK` returns to the watch face. The UI creates and destroys the existing LVGL
popup strictly from the snapshot; input code does not call LVGL. Core tests,
simulator coverage, Diagnostic board acceptance, and CDC popup visibility are
the closure evidence.

## Next round: M9a task ownership

Move runtime bootstrap ownership to `defaultTask`, which starts required runtime
tasks and exits. UI ownership moves out of the USB service, and the UI/app owner
becomes the sole consumer of the service event queue. RTC/time work remains the
separate M9b round.

## Risks and blockers

- The signed M2 `security_counter` is not yet persisted or compared with a
  confirmed value. Metadata, trial confirmation, and replay rejection begin in
  M17-M19.
- Editor regeneration remains a manual LVGL Pro Editor operation. CI builds the
  committed generated C and does not replace the generator.
- M8 lacks a real core-driven popup interaction path.
- M9 still starts the UI from the USB service and keeps `defaultTask` alive.
- LSM6DS3 is the only closed sensor; missing sensors remain diagnostic degraded
  states rather than accepted sensor support.
- The no-battery board cannot establish KEY_WAKE, physical power-latch polarity,
  external watchdog wiring, or measured current reduction.
- W25Q128, resource storage, KT6368 transport, OTA metadata, install recovery,
  and rollback are not implemented.

## Latest verification

- Debug App and Bootloader build, format check, Cppcheck, and all eight Manifest
  host tests passed.
- A valid fixed-size signed App package passed host verification against the
  Bootloader public key.
- Four independently mutated packages were rejected by both host validation and
  the board: image digest mismatch, wrong board ID, out-of-range image length,
  and nonzero invalid ECDSA signature.
- For each board rejection, the target CDC device remained absent for eight
  seconds and the halted PC was `0x080002D4` inside the Bootloader region.
- Each rejection was followed by writing and verifying the valid package. The
  restored App returned CDC `ping`, `info`, `health`, `stats`, and `diag`; it
  reported 240x280, runtime stage 3, no stored diagnostic capsule, and zero
  observed CDC drops.
- A 64 KiB readback before and after the acceptance sequence produced identical
  Bootloader SHA-256 values. Only the App slot was erased and programmed.
