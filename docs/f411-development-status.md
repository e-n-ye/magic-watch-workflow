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

- This M15 USB resource update began from clean `origin/main` at `18815c2`
  (the merged M14 littlefs/resource-storage baseline).
- The original pre-relocation Debug App reference was Flash `64,340 B` and RAM
  `30,208 B`. The current practical App budget remains 400 KiB Flash and 128 KiB
  RAM.
- Fourteen of 21 numbered milestones are fully closed: M0-M9, M11, and M13-M15. M10b LIS2MDL,
  M10c AHT20-compatible, M10d MAX30102, and M10e CW2015 driver/service paths
  plus the M10f aggregation path are implemented. M10 remains partial because
  the board still lacks independent LIS2MDL and CW2015 physical acceptance.
  This is about 67% by strict numbered scope. The current artifact is a
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
| M14 | littlefs and resource streaming | Complete. Fixed raw partitions, App-only littlefs, bounded resource reads, host tests, and board persistence acceptance are closed. |
| M15 | USB diagnostic/log/resource protocol | Complete; fixed MWRP framing, ACK/NACK, CRC32, SHA-256, bounded paths, LittleFS temporary writes, atomic rename, and board acceptance are closed. |
| M16 | KT6368 SPP transport | Not started. |
| M17 | YModem candidate download and package verification | Not started. |
| M18 | Backup, install, and power-loss recovery | Not started. |
| M19 | Trial confirmation and rollback | Not started. |
| M20 | Final configurations, simulator, fault injection, budgets, regression report | Not started. |

## Current round: M15 USB atomic resource transfer

M15 keeps the M14 raw map and adds the fixed little-endian `MWRP` frame contract:
`MWRP/version/type/flags/sequence/payload-length/CRC32`, with a 512-byte maximum
payload. `BEGIN` carries a bounded absolute littlefs path, size, and SHA-256;
ordered `DATA` frames stream to a `*.upload` temporary file; `COMMIT` verifies
the digest and atomically renames; `ABORT`, CRC errors, sequence/offset errors,
storage failures, and digest mismatches remove the temporary file. Existing
ASCII CDC diagnostics remain available when the binary protocol is idle. No
MSC mode and no OTA partition access were added.

## Next round: M16 KT6368 SPP transport

M16 will verify KT6368 enable polarity, SPP pairing, USART1 DMA+IDLE ring
transport, and recovery from disconnects. It will carry the same signed-package
bytes only after the board facts are established; no unsupported AT command or
phone application is planned.

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
- A full 64 KiB ST-Link flash read of the Bootloader has non-deterministic
  bytes outside its executable prefix across reset cycles, even while the core
  is halted and IWDG debug-freeze is enabled. The successful M14 command wrote
  and verified only the App slot, and the Bootloader executable prefix still
  matches the local build, but a reliable whole-region hash procedure remains
  an open debug-tooling issue.
- KT6368 transport, OTA metadata, install recovery, and rollback are not
  implemented. M15 does not provide them implicitly.

## Latest verification

- Debug, Diagnostic, and Release App builds passed. Debug format-check and
  Cppcheck passed; Host CTest passed `15/15` and the 240x280 simulator smoke
  test passed `1/1`.
- Current App budgets remain within the 400 KiB practical Flash budget:
  Debug Flash `333,872 B` / RAM `91,572 B`; Diagnostic Flash `343,412 B` /
  RAM `91,572 B`; Release Flash `182,132 B` / RAM `91,560 B`. Bootloader is
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
  and bus-error recovery. LittleFS host tests cover the fixed partition map,
  format/mount persistence, bounded image/font/text reads, and unmounted and
  invalid-reader paths. M15 protocol tests cover split input, valid multi-frame
  transfer, CRC/version/flags/path/sequence/offset/state rejection, digest
  mismatch, and ABORT. Host CTest passed `15/15`.
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
- On the connected W25Q128, `fs-test` returned `result=ok`, `mounted=1`, and
  image/font/long-text chunk counts `3/3/8` for the fixed
  `0x110000-0xFFFFFF` partition. After a board reset, `fs` returned
  `mounted=1 result=ok mount=ok`, proving persistent remount without accessing
  OTA partitions.
- The M15 Debug App was packed as signed firmware version/counter `19` and
  host-verified with the external signing key; the derived public key matched
  the Bootloader constant. ST-Link/OpenOCD wrote and verified the complete App
  slot at `0x08010000`. Dynamic USB CDC probing found the target and accepted a
  733-byte resource in two DATA frames, followed by COMMIT. A corrupted CRC
  received NACK error `4`; a wrong SHA-256 received NACK error `12`. After a
  reset, `ping`, `info`, `fs`, and `stats` were valid; `fs` remounted the fixed
  littlefs partition and `stats` showed zero CDC drops.
