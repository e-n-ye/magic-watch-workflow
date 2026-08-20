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

- The current baseline is `origin/main=7fab1f5`, confirmed after the M19
  trial/rollback PR was rebased and merged. The worktree keeps the pre-existing
  local stash untouched.
- Eighteen of 21 numbered milestones are fully closed: M0-M9, M11, M13-M17,
  M19, and M20. M10, M12, and M18 remain partial. This is about 86% by strict
  numbered scope. The artifact is a runnable and diagnosable reference
  firmware with board-accepted signed install, trial confirmation, and
  automatic-reset rollback flow; known hardware and power-loss limits remain
  explicit below.
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
| M20 | Final configurations, simulator, fault injection, budgets, regression report | Complete as the final reference-firmware regression report. Debug/Diagnostic/Release, host tests, simulator, manifest, map/size budgets, host fault models, and connected-board CDC regression passed; deliberate M18 power-loss injection remains a documented limitation. |

## Current round: M20 final regression and reference report

M20 is accepted for the current reference baseline. The final matrix passed:

```text
Debug/Diagnostic/Release build       pass
Debug format-check + cppcheck       pass
Host CTest                           21/21
240x280 UI simulator                 1/1
Signed manifest host tests           8/8
ELF/map origin and budget checks     pass
```

The connected board was checked through dynamically discovered USB CDC without
writing a new image. It reported the frozen 240x280 profile, runtime health
stage 3, `encoder=1`, `touch=1`, CST816 `0xb5`, W25Q128 `0xef4018`, mounted
littlefs, and zero CDC RX/TX drops. The resource test completed with image/font
and long-text chunk counts `3/3/8`; LSM6DS3, AHT20, and MAX30102 produced valid
samples, while LIS2MDL and CW2015 remained the documented degraded devices.
Health stayed at stage 3 for 30 seconds with increasing service heartbeats.

The accepted M19 candidate remains confirmed at security counter `26`:

```text
state=confirmed confirmed=26 candidate=26 version=26 image=360084 trial=0 error=0
```

The M19 automatic-reset rollback sequence, pure-C trial/fault model, and
metadata/install failure-injection tests remain part of the accepted evidence.
No forced HardFault or deliberate erase/write power-loss was claimed in the
M20 board run.

## Next round: maintenance only

No numbered implementation round remains. Any future change must start from
the latest `origin/main`, use a focused branch/PR, and rerun this final matrix.

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
- Deliberate M18 erase/write power-loss injection was not performed because the
  connected workstation had no available ST-Link/OpenOCD/STM32 programmer CLI;
  the host fault model and all non-power-loss board paths remain accepted.
- This remains a reference firmware, not a production secure-OTA product.

## Latest verification

- Debug App: `360044 B` flash / `96308 B` RAM; Diagnostic App: `369584 B` /
  `96308 B`; Release App: `195440 B` / `96296 B`.
- Debug/Diagnostic Bootloader: `18664 B` flash / `1208 B` RAM; Release
  Bootloader: `11740 B` flash / `1208 B` RAM. All are within the configured
  400 KiB App and 64 KiB Bootloader flash budgets and 128 KiB RAM budget.
- Host CTest passed `21/21`, the 240x280 simulator passed `1/1`, and signed
  manifest tests passed `8/8`.
- M20 board checks used dynamic USB CDC and recorded only summarized facts; no
  private key, device path, temporary image, dump, or complete serial log is in
  the repository.
