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

- The current baseline is `origin/main=90c34c3`, confirmed before the M19
  trial/rollback branch was created. The worktree keeps the pre-existing
  local stash untouched.
- Seventeen of 21 numbered milestones are fully closed: M0-M9, M11, M13-M17,
  and M19. M10, M12, and M18 remain partial; M20 is not started. This is about
  81% by strict numbered scope. The artifact is a runnable and diagnosable
  vertical slice with board-accepted signed install, trial confirmation, and
  automatic-reset rollback flow; deliberate power-loss recovery and final
  regression remain open.
- Current hardware facts include STM32F411, 24 MHz HSE, ST-Link-compatible SWD,
  ST7789, CST816, W25Q128, KT6368, and the schematic EEPROM candidate
  `BL24C02F-RRRC`. The read-only probe saw responses at `0x50` and `0x57`; the
  populated part and address strap remain unconfirmed.

## Milestones

| ID | Scope | Current status |
| --- | --- | --- |
| M0 | Rolling status page and baseline | Complete; this page is the current-state source and README links it. |
| M1 | Bootloader, App relocation, VTOR, flash/debug flow | Complete. |
| M2 | Manifest, trailer, host packaging, rejection paths | Complete for the signed factory-package contract and four board rejection paths. |
| M3 | Budgets, assertions, reset capsule, CDC logs, Diagnostic | Complete. |
| M4 | Pure-C core and host tests | Complete. |
| M5 | CST816, encoder, button, normalized input | Complete. |
| M6 | LVGL 9.5, DMA flush, UI task, 240x280 budget | Complete. |
| M7 | Editor XML, generated C, PC simulator | Complete for the approved manual Editor export workflow. |
| M8 | Six page categories, lifecycle, interaction, leak pressure | Complete; the core-driven Diagnostic popup and physical input path are accepted. |
| M9 | RTC/time service, queues, heartbeats, init policy | Complete. |
| M10 | Five sensor drivers and aggregation | Partial; LSM6DS3, AHT20-compatible, and MAX30102 are available on the board. LIS2MDL and CW2015 remain explicitly degraded; the LIS2MDL auxiliary-I2C instability is a known hardware limitation. |
| M11 | EEPROM part/address facts | Complete as a read-only fact probe; the exact part and strap remain a documented risk. |
| M12 | Power and watchdog | Partial; software Stop/wake, display-off, software-off, and IWDG paths have board evidence. KEY_WAKE, physical cutoff, watchdog wiring, and current remain unverified on the no-battery board. |
| M13 | W25Q128 raw driver | Complete; JEDEC ID, page program, sector erase, timeout, and recovery paths are accepted. |
| M14 | littlefs and resource streaming | Complete; fixed partitions and chunked image/font/text reads are accepted. |
| M15 | USB diagnostic/log/resource protocol | Complete; MWRP framing, integrity checks, bounded paths, atomic rename, and board acceptance are closed. |
| M16 | KT6368 SPP transport | Complete for the USART1 DMA+IDLE bridge and recovery paths; an independent long-run Bluetooth transfer remains limited evidence. |
| M17 | YModem candidate download and package verification | Complete for USB CDC; the board accepted a full signed package and persisted `candidate-ready`. |
| M18 | Backup, install, and power-loss recovery | Partial; M18a host/fault model and M18b board backup/install are accepted. Deliberate interruption at erase/write boundaries and resume-after-power-loss remain open. |
| M19 | Trial confirmation and rollback | Complete for the pure-C transition model and connected-board healthy confirmation/automatic-reset sequence. HardFault capsule-to-pending-rollback is covered by code and host tests; deliberate HardFault injection is not claimed. |
| M20 | Final configurations, simulator, fault injection, budgets, regression report | Not started. |

## Current round: M19 trial confirmation and rollback

M19 is accepted for the connected board. A signed candidate (version/counter
`26`, image length `360084`) was downloaded over dynamic USB CDC with all
`448/448` 1 KiB data blocks acknowledged. After installation, the App reported
`encoder=1`, `touch=1`, `chip=0xb5`, and healthy runtime stage 3. After more than
30 seconds, metadata reached:

```text
state=confirmed confirmed=26 candidate=26 version=26 image=360084 trial=0 error=0
```

The same candidate was staged again and the board was reset three times before
the 30-second confirmation window. The next startup completed the automatic
rollback flow and returned to `state=confirmed`, with `trial=0` and the
confirmed counter intact. This board run does not claim a forced HardFault or
power-loss injection: the HardFault capsule path and watchdog policy are
covered by code and host tests. Missing/degraded sensors remained non-blocking.

## Next round: M20 final regression and reference report

Run the final Debug/Diagnostic/Release, simulator, resource-budget, fault-model,
and whole-device regression matrix. Deliberate M18 erase/write power-loss
injection remains a known open acceptance item.

## Risks and blockers

- The signed `security_counter` policy is persisted and compared; replay and
  rollback are now covered by the M19 transition model and board run.
- The no-battery board cannot establish KEY_WAKE, physical cutoff, external
  watchdog wiring, or a measured current target.
- LIS2MDL is routed through the LSM6DS3 auxiliary Sensor Hub and currently
  returns no valid identity/sample. It remains a degraded hardware fact, not a
  software acceptance failure.
- The exact AHT20/AHT21 package marking, CW2015 population/routing, and EEPROM
  part/strap are not proven by the available hardware evidence.
- Editor regeneration remains a manual LVGL Pro Editor operation. CI builds
  committed generated C and does not replace the generator.
- CDC diagnostic output can drop when no host reader is attached; the accepted
  board checks used an active reader and observed zero drops.
- M18 power-loss injection and the M20 final regression report are not yet
  complete, so the firmware is not presented as a complete secure OTA product.

## Latest verification

- Debug configure/build, `format-check`, and `cppcheck` passed. The current
  Debug App image is `360044 B`; the Bootloader executable is `18664 B` with
  `1208 B` static RAM usage.
- Host CTest passed `21/21`; the 240x280 simulator smoke test remains `1/1`.
- The M19 board run used dynamic USB CDC discovery, verified W25Q128 JEDEC
  `0xef4018`, accepted the signed candidate, confirmed it after 30 seconds,
  and completed the forced three-reset rollback sequence. No private key,
  device path, temporary image, dump, or complete serial log is part of the
  repository.
