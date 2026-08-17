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

- This M10e CW2015 update began from clean `origin/main` at `a5b857b`
  (`feat:f411:add MAX30102 sensor service`).
- The original pre-relocation Debug App reference was Flash `64,340 B` and RAM
  `30,208 B`. The current practical App budget remains 400 KiB Flash and 128 KiB
  RAM.
- Ten of 21 numbered milestones are fully closed: M0-M9. M10b LIS2MDL,
  M10c AHT20-compatible, M10d MAX30102, and M10e CW2015 driver/service paths
  are implemented, while the five-sensor M10 aggregate is still partial. This
  is about 48% by strict numbered scope. The current artifact is a runnable,
  diagnosable vertical slice; upgrade, power-loss recovery, and rollback are
  not present.
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
| M10 | Five sensor drivers and aggregation | Partial; LSM6DS3, LIS2MDL, AHT20-compatible, MAX30102, and CW2015 driver/service paths are implemented. M10c board evidence confirms AHT20/21 response and valid samples; M10d remains operational in the current board image. CW2015 software and negative-probe behavior are verified, but the no-battery board does not answer at `0x62`; aggregation remains for M10f. |
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

## Current round: M10e CW2015

M10e is complete as a pure-C, read-only CW2015 version/VCELL/SOC decode and
service path, direct I2C1 board adapter at the legacy `0x62` address, CDC
diagnostic consumer, and Host test. It deliberately does not write BATINFO,
configure a battery curve, or restart the fuel gauge. On the connected board,
the service reports `version=0x00`, `sample=0`, `errors=1`, and `state=2` after
the `0x62` probe; with no battery fitted, this is a confirmed negative probe,
not a software or CW2015 physical acceptance. Sensor aggregation remains the
separate M10f round.

## Next round: M10f sensor aggregation

M10f will add the bounded aggregation consumer, latest-status/availability in
the core snapshot, and explicit degraded state for absent sensors. It must not
turn the CW2015 no-battery result or the known LIS2MDL auxiliary-I2C issue into
false hardware success.

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
- The no-battery board cannot establish KEY_WAKE, physical power-latch polarity,
  external watchdog wiring, or measured current reduction.
- W25Q128, resource storage, KT6368 transport, OTA metadata, install recovery,
  and rollback are not implemented.

## Latest verification

- Debug, Diagnostic, and Release App builds passed. Debug format-check and
  Cppcheck passed; Host CTest passed `10/10` and the 240x280 simulator smoke
  test passed `1/1`.
- Current App budgets remain within the 400 KiB practical Flash budget:
  Debug Flash `274,188 B` / RAM `83,640 B`; Diagnostic Flash `283,604 B` /
  RAM `83,640 B`; Release Flash `147,404 B` / RAM `83,632 B`. Bootloader is
  `6,564 B` Flash / `1,056 B` RAM.
- M10b host tests cover Sensor Hub bank selection, LIS2MDL address/configuration,
  `sensor_hub_end_op` timeout, NACK handling, identity, sample decoding,
  periodic service behavior, and event-drop accounting.
- M10c Host tests cover AHT20/21 CRC validation, fixed-point decode, initialize
  and trigger command order, busy timeout, read failure, and event-drop
  accounting. M10d MAX30102 is merged at `a5b857b`; the current board image
  still reports the LSM6DS3, AHT20-compatible, and MAX30102 paths without
  regressions. M10e Host tests cover CW2015 voltage/SOC decode, periodic reads,
  invalid SOC, read failure, and event-drop accounting. A signed Diagnostic
  version 12/counter 12 package passed host verification, was written to and
  verified at `0x08010000`, and did not change the Bootloader. A second halted
  read confirmed the before/after Bootloader SHA-256 as
  `59034D2B990BFA044A1A928551E17828365D99ACB950D33F7E68371C9107FEB1`.
- After dynamic CDC re-enumeration, `ping`, `info`, `health`, `stats`, `diag`,
  `sensor`, and `power` were valid. Health was stage 3 with app, UI, USB, and
  sensor services healthy; the queue and CDC drop counters were zero and no
  diagnostic capsule was present. The CW2015 line was
  `sensor cw2015=0 version=0x00 sample=0 count=0 errors=1 invalid_soc=0 drop=0`
  `voltage_mv=0 soc=0 fraction=0 state=2`; no valid battery sample was
  claimed.
- Board sensor evidence: LSM6DS3 was ready with ID `0x6A`, `3,554` samples,
  and zero read/event drops. The current AHT20-compatible sample was
  `32.94 C` and `65.21%` relative humidity with zero errors. MAX30102 was ready
  with part `0x15`, revision `0x03`, `1,826` samples, zero read/ID/reset/FIFO
  errors, and finger detection active. LIS2MDL remained `ready=0`, `id=0x00`,
  sample count `0`, read errors `1`, NACK count `0`, and state `7`; this is
  recorded as the authorized Sensor Hub/auxiliary-I2C hardware risk, not as a
  passing magnetometer acceptance.
- Board AHT20/21 evidence: the direct `0x38` path reported `ready=1`,
  `cal=1`, valid samples with counts increasing from `131` to `304`, and zero
  read errors, CRC errors, busy timeouts, or event drops. Representative
  readings were `33.76 C` and `66.26%` relative humidity; status returned
  `0x18` when ready. The dynamic CDC reader also confirmed health stage 3,
  zero queue/CDC drops, `diag=none`, and normal power/watchdog state.
