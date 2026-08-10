# F411 Development Status

This is the rolling status page for the STM32F411 reference firmware. It is
intentionally current-state only; Git commits and pull requests contain the
historical process record. The numbered scope is defined in
[f411-development-plan.md](f411-development-plan.md).

## End state

The target is a real, runnable, diagnosable, upgradable, and rollbackable F411
watch reference firmware with a 240x280 UI, external resources, and signed OTA.
It is a reference implementation, not a production-product claim. The fixed
order is hardware facts, pure-C contracts and host tests, board integration,
UI/resources, power, then secure OTA. No empty placeholder modules are added.

## Frozen decisions

| Area | Decision |
| --- | --- |
| F411 display | ST7789, 240x280; LilyGo 240x240 is an independent future profile |
| Core | Pure-C `watch_core`; no HAL, FreeRTOS, or LVGL dependency |
| UI ownership | One UI task owns LVGL and the core; services use bounded queues |
| Page lifetime | Maximum depth four; one page tree and one popup; page controls are destroyed on exit |
| Input | Touch click/left-edge swipe-back plus normalized encoder, button, `BACK`, and `WAKE` events |
| LVGL | Pinned 9.5.0; ST7789 SPI1 DMA; F411 uses `LV_USE_XML=0` |
| XML | LVGL Pro Editor is the formal source; manual Editor Code/export produces committed C; generated files are never hand-edited; no Pro CLI is required |
| Boot layout | Bootloader `0x08000000-0x0800FFFF`; App slot `0x08010000-0x0807FFFF` |
| App budget | 448 KiB slot, final 4 KiB signed trailer, practical CI budget 400 KiB |
| OTA security | SHA-256 plus ECDSA P-256, board/version checks, security counter, trial confirmation, rollback |
| External flash | W25Q128 metadata, candidate, rollback, and littlefs partitions remain separate |
| Bluetooth | KT6368 is only a transparent USART1 `115200 8N1` SPP transport |
| USB | CDC diagnostics, logs, and resource transfer; no online MSC |
| Security boundary | Integrity and replay protection only; no claim against SWD, unlocked RDP, or physical flash tampering |

## Baseline

- Reconfirmed baseline: `ce3162c` (`origin/main` and local `main` agree before
  this lifecycle round). The last functional firmware evidence remains `afd7d0f`.
- PRs #5 through #28 are merged with Rebase and merge, and each required CI Gate
  passed. The worktree was clean before this CI change.
- The original pre-relocation Debug App reference was Flash `64,340 B` and RAM
  `30,208 B`. Later rounds increased the image while remaining below the current
  400 KiB App and 128 KiB RAM limits.
- Current board facts include STM32F411, 24 MHz HSE, ST-Link-compatible SWD,
  ST7789, CST816, W25Q128, and KT6368 UART wiring. EEPROM part and address are
  not yet established from board evidence.

## Completion assessment

The earlier page incorrectly compressed M0-M20 into M0-M15 and treated partial
sensor, runtime, and page work as complete. Against the numbered plan, the
project is approximately 40-45% complete by closed-loop scope, not by lines of
code. M0-M7 have usable evidence, M8-M10 are partial, M11 is an open hardware
fact, and the power behavior from the historical M11 work belongs to M12 in the
numbered plan. W25Q128 storage through final OTA acceptance has not started.

## Milestones

| ID | Scope | Current status and evidence |
| --- | --- | --- |
| M0 | Rolling status page and baseline | Complete; this page and the plan are linked from README; CI Gate history is present. |
| M1 | Bootloader, App relocation, VTOR, flash/debug flow | Complete; combined and App-only OpenOCD programming and cold-start behavior were accepted. |
| M2 | Manifest, trailer, host packaging, rejection paths | Software path is implemented; negative-path board acceptance and key-rotation evidence remain open. |
| M3 | Budgets, assertions, reset capsule, CDC logs, Diagnostic | Complete for the recorded Debug/Diagnostic checks and board diagnostic acceptance. |
| M4 | Pure-C core and host tests | Complete; core contracts and host CTest are present. |
| M5 | CST816, encoder, button, normalized input | Complete; click, swipe-back, debounce, and board input acceptance passed. |
| M6 | LVGL 9.5, DMA flush, UI task, 240x280 budget | Complete; Debug/Diagnostic budgets and focused board page acceptance passed. |
| M7 | Editor XML, generated C, PC simulator | Complete for the approved manual Editor export workflow; host simulator smoke passed. F411 does not parse XML at runtime. |
| M8 | Six page categories, lifecycle, interaction, leak pressure | Partial; four-page navigation and a 32-cycle lifecycle pressure smoke now pass, but resource viewer, hidden diagnostics, popup coverage, and the complete six-category page set remain. |
| M9 | RTC/time service, queues, heartbeats, init policy | Partial; runtime health and bounded queues exist, but there is no complete RTC/time service contract and `defaultTask` still remains a permanent loop. |
| M10 | Five sensor drivers and aggregation | Partial; only LSM6DS3 has a driver/service path. LIS2MDL, AHT20, MAX30102, CW2015, and aggregation remain. |
| M11 | EEPROM part/address facts | Not started; the part and address are unconfirmed. Do not migrate the old 24LC32 driver. |
| M12 | Power and watchdog | Behavior implemented ahead of its numbered round in the historical M11 work: RTC-only Stop/wake, display-off, software-off, and IWDG passed current-board acceptance. KEY_WAKE and physical power cutoff remain unverified because the board has no battery; no current target is claimed. |
| M13 | W25Q128 raw driver | Not started. |
| M14 | littlefs and resource streaming | Not started. |
| M15 | USB diagnostic/log/resource protocol | Not started as the planned atomic resource protocol; existing CDC diagnostics are not this milestone. |
| M16 | KT6368 SPP transport | Not started; enable polarity, pairing, sustained DMA+IDLE transfer, and recovery need board evidence. |
| M17 | YModem candidate download and package verification | Not started. |
| M18 | Backup, install, and power-loss recovery | Not started. |
| M19 | Trial confirmation and rollback | Not started. |
| M20 | Final configurations, simulator, fault injection, budgets, regression report | Not started. |

## Current round: M8a lifecycle pressure

This round extends the 240x280 simulator smoke path across the currently
implemented WATCHFACE, LAUNCHER, STATUS, and SETTINGS pages. It repeats entry
and exit through the page stack 32 times and checks exact page creation and
destruction counts (199 created, 198 destroyed before final deinit). The
simulator consumes each core command as the UI task would; no firmware,
storage, sensor, or OTA behavior is added.

## Next round

Complete M8b by adding the resource viewer, hidden diagnostics page, popup
lifecycle, and the remaining interaction coverage. Keep the M8 page set and
leak checks visible in the simulator before starting the separate M9 runtime
contract round. Do not begin W25Q128 or OTA modules until the M8-M10 contracts
and their test gates are visible in CI.

## Risks and blockers

- M7 regeneration is manual and depends on a local LVGL Pro Editor project. CI
  builds committed generated C but does not regenerate it or require a Pro
  token. Editor export failure blocks regeneration and must not trigger a
  second generator.
- M2's security counter is signed but is not persisted and compared with
  confirmed metadata until the later OTA rounds.
- The EEPROM model, address, and electrical behavior must be confirmed from the
  actual board. Legacy 24LC32 code is not evidence.
- M8 still lacks the full page set, popup lifecycle, and interaction coverage;
  the current four-page pressure/leak smoke is now recorded above.
- M9's runtime health checks do not yet constitute the planned RTC/time service;
  application code still has direct tick-time dependencies and `defaultTask`
  ownership needs correction.
- M10 has only LSM6DS3. Its wiring/address are from the V2.1 reference project;
  interrupt paths are not validated, and a missing sensor remains a diagnostic
  degraded state rather than a passing sensor acceptance.
- The no-battery board proves RTC Stop/wake behavior only. KEY_WAKE, physical
  power-latch polarity, the external WDI/WDOG_EN circuit, and measured current
  reduction remain unverified.
- The reset capsule does not survive a complete power loss; backup metadata is
  deferred to the storage/OTA rounds.
- CI now runs host CTest and the 240x280 simulator for product/test changes. Any
  future source tree or test-root addition must update the path classifier and
  preserve the explicit host result check in `CI / CI Gate`.

## Latest verification

The last merged functional evidence is recorded at `afd7d0f`:

- M11 host tests passed the power-state transition and health-gated watchdog
  checks together with the existing core, runtime, and LSM6DS3 suites.
- The Debug F411 build, format-check, and Cppcheck passed; the linked Debug App
  was reported as Flash `254,304 B` and RAM `83,096 B`.
- Board acceptance used OpenOCD on the current no-battery board. USB CDC showed
  `power`, `display-off`, `sleep`, and `shutdown`; RTC Stop/wake returned after
  roughly three seconds, USB re-enumerated, and the UI remained usable. Software
  off blanked the display and required reset for recovery. KEY_WAKE was not
  electrically tested.
- The M8a simulator pressure smoke passed locally: `display=240x280`,
  `pages=4`, `lifecycle_cycles=32`, `creates=199`, `destroys=198`, and
  `active=WATCHFACE`. The final deinit check also returned creation and
  destruction counts to equality.
- The preceding CI round passed locally with four host CTest cases and one
  simulator smoke test. The required F411 Debug configure/build, format-check,
  and Cppcheck also passed; no board flashing was needed for a CI-only change.
- Earlier M7-M10 host and board evidence remains valid where listed in Git
  history, but it does not promote the partial milestones above to complete.

This CI branch must pass `git diff --check`, the targeted UTF-8/garbled-text
check, the F411 validation commands, both host CTest suites, and the repository
`CI / CI Gate`.
