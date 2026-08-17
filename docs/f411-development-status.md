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

- This M13 W25Q128 update began from clean `origin/main` at `ae5fb77`
  (`fix:f411:restore USB CDC after Stop wake`, merged PR #43).
- The original pre-relocation Debug App reference was Flash `64,340 B` and RAM
  `30,208 B`. The current practical App budget remains 400 KiB Flash and 128 KiB
  RAM.
- Twelve of 21 numbered milestones are fully closed: M0-M9, M11, and M13. M10b LIS2MDL,
  M10c AHT20-compatible, M10d MAX30102, and M10e CW2015 driver/service paths
  plus the M10f aggregation path are implemented. M10 remains partial because
  the board still lacks independent LIS2MDL and CW2015 physical acceptance.
  This is about 57% by strict numbered scope. The current artifact is a
  runnable, diagnosable vertical slice; upgrade, power-loss recovery, and
  rollback are not present.
- Current hardware facts include STM32F411, 24 MHz HSE, ST-Link-compatible SWD,
  ST7789, CST816, W25Q128, KT6368, and a schematic EEPROM candidate
  `BL24C02F-RRRC`. A read-only scan saw ACK at `0x50` and `0x57`; the exact
  populated part and address strap remain unconfirmed.

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
| M10 | Five sensor drivers and aggregation | Partial; all five driver/service paths and M10f pure-C aggregation are implemented. The board reports LSM6DS3/AHT20/MAX30102 available and explicitly degrades LIS2MDL/CW2015; physical acceptance of those two missing devices remains open. |
| M11 | EEPROM part/address facts | Complete as a read-only fact probe; `0x50` and `0x57` responded, but the exact part/strap remains a documented risk and no legacy driver was migrated. |
| M12 | Power and watchdog | Partial; RTC Stop/wake, display-off, software-off, and IWDG have software and no-battery board evidence. KEY_WAKE, physical cutoff, watchdog wiring, and current remain open. |
| M13 | W25Q128 raw driver | Complete; pure-C protocol, SPI3 board adapter, host fault tests, and explicit board read/program/erase acceptance are closed. |
| M14 | littlefs and resource streaming | Not started. |
| M15 | USB diagnostic/log/resource protocol | Not started; existing CDC diagnostics are not the atomic resource protocol. |
| M16 | KT6368 SPP transport | Not started. |
| M17 | YModem candidate download and package verification | Not started. |
| M18 | Backup, install, and power-loss recovery | Not started. |
| M19 | Trial confirmation and rollback | Not started. |
| M20 | Final configurations, simulator, fault injection, budgets, regression report | Not started. |

## Current round: M13 W25Q128 raw driver

M13 adds the reusable pure-C W25Q128 protocol layer with JEDEC ID, status
polling, bounded reads, page-program, 4 KiB sector erase, timeout, and bus-error
results. The F411 board adapter owns SPI3 and CS HAL calls. CDC `w25` reports
identity/readiness; explicit `w25-test` verifies erase, page-program, readback,
verification, and cleanup on one test sector. littlefs and OTA metadata remain
unmounted and unchanged.

## Next round: M14 littlefs and resource streaming

M14 will fix the raw partitions, mount littlefs only in the App, and prove that
images, fonts, and long text can be read in bounded chunks while Bootloader OTA
partitions remain raw and unmounted.

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
- The legacy board project identifies CW2015 at `0x62`, but the connected
  no-battery board returned no response. Power, population, and PCB routing
  remain unconfirmed; the new service stays read-only until those facts are
  established.
- The M11 EEPROM probe returned ACK at both `0x50` and `0x57`. This is not
  enough to identify the populated part or strap state, so the old 24LC32
  driver remains intentionally unmigrated.
- The no-battery board cannot establish KEY_WAKE, physical power-latch polarity,
  external watchdog wiring, or measured current reduction.
- Resource storage, KT6368 transport, OTA metadata, install recovery, and
  rollback are not implemented. The M13 W25Q128 raw layer deliberately does
  not provide those higher-level behaviors.

## Latest verification

- Debug, Diagnostic, and Release App builds passed. Debug format-check and
  Cppcheck passed; Host CTest passed `13/13` and the 240x280 simulator smoke
  test passed `1/1`.
- Current App budgets remain within the 400 KiB practical Flash budget:
  Debug Flash `283,672 B` / RAM `86,000 B`; Diagnostic Flash `293,212 B` /
  RAM `86,000 B`; Release Flash `152,584 B` / RAM `85,992 B`. Bootloader is
  `6,564 B` Flash / `1,056 B` RAM.
- M10b host tests cover Sensor Hub bank selection, LIS2MDL address/configuration,
  `sensor_hub_end_op` timeout, NACK handling, identity, sample decoding,
  periodic service behavior, and event-drop accounting.
- M10c Host tests cover AHT20/21 CRC validation, fixed-point decode, initialize
  and trigger command order, busy timeout, read failure, and event-drop
  accounting. M10d MAX30102 is merged at `a5b857b`; M10e Host tests cover
  CW2015 voltage/SOC decode, periodic reads, invalid SOC, read failure, and
  event-drop accounting. M10f Host tests cover aggregate availability,
  degraded-state calculation, revision coalescing, snapshot validation, and
  core sensor-status commands. M11 Host tests cover the read-only EEPROM probe
  address window, response mask, completion, and invalid-argument handling. A
  W25Q128 host tests cover JEDEC ID, chunked reads, page-boundary validation,
  sector-alignment/range validation, write-enable sequencing, ready timeout,
  and bus-error recovery. Host CTest passed `13/13`.
- After dynamic CDC re-enumeration, `ping`, `info`, `eeprom`, `health`,
  `stats`, `diag`, `sensor`, and `power` were valid. Health was stage 3 with
  app, UI, USB, and sensor services healthy; the queue was `2` during the read,
  with zero RX/TX drops and `diag=none`. The EEPROM line reported
  `complete=1`, `range=0x50-0x57`, `probed=8`, and `response_mask=0x81`.
  `info` reported `sensors=0x15 degraded=1`. Board sensor evidence was LSM6DS3
  ready with ID `0x6A`, AHT20-compatible `30.51 C` / `71.79%`, and MAX30102
  ready with part `0x15`, revision `0x03`, and finger detection active. CW2015
  and LIS2MDL remained explicitly degraded with no valid samples; these are
  hardware facts, not false acceptance.
- The signed Debug version `16`/counter `16` package was host-verified and
  written only to the App slot at `0x08010000`; the Bootloader was not written.
  Dynamic CDC returned `w25 id=0xef4018 id_result=ok ready=ok`, and the explicit
  test returned `w25test addr=0x00f000 id=0xef4018 id_result=ok erase=ok
  program=ok read=ok verify=1 cleanup=ok`.
