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

- 本次审查的实现基线为 `origin/main=9bceb0e`，对应已合并的 M20 PR #58。
  本文档自身未来的合并提交不作为新的实现基线。
- Strict status is: M0-M9, M11, and M13-M17 are `完成`; M10 and M12 are
  `按限制封板`; M18 and M19 are `待板测`; M20 is `条件通过`. The firmware is
  runnable and diagnosable, but the final claim remains gated by the destructive
  board evidence and the C2 final matrix.
- Current hardware facts include STM32F411, 24 MHz HSE, ST-Link-compatible SWD,
  ST7789, CST816, W25Q128, KT6368, and the schematic EEPROM candidate
  `BL24C02F-RRRC`. The read-only probe saw responses at `0x50` and `0x57`; the
  populated part and address strap remain unconfirmed.

## Milestones

| ID | Scope | Current status |
| --- | --- | --- |
| M0 | Rolling status page and baseline | 完成：本页是当前状态来源，README 已链接。 |
| M1 | Bootloader, App relocation, VTOR, flash/debug flow | 完成。 |
| M2 | Manifest, trailer, host packaging, rejection paths | 完成：签名工厂包契约和四类板端拒绝路径已验证。 |
| M3 | Budgets, assertions, reset capsule, CDC logs, Diagnostic | 完成。 |
| M4 | Pure-C core and host tests | 完成。 |
| M5 | CST816, encoder, button, normalized input | 完成。 |
| M6 | LVGL 9.5, DMA flush, UI task, 240x280 budget | 完成。 |
| M7 | Editor XML, generated C, PC simulator | 完成：采用已批准的手动 Editor 导出流程。 |
| M8 | Six page categories, lifecycle, interaction, leak pressure | 完成：core 驱动的 Diagnostic 弹窗和物理输入路径已验收。 |
| M9 | RTC/time service, queues, heartbeats, init policy | 完成。 |
| M10 | Five sensor drivers and aggregation | 按限制封板：LSM6DS3、AHT20-compatible 和 MAX30102 可用；LIS2MDL、CW2015 保持明确降级。不新增改板、电池、电流指标或器件返修范围。 |
| M11 | EEPROM part/address facts | 完成：只读事实探针已完成，具体型号和地址 strap 仍作为风险记录。 |
| M12 | Power and watchdog | 按限制封板：软件 Stop/wake、息屏、软件关机和 IWDG 路径已有证据；不新增改板、电池、电流指标或器件返修范围。 |
| M13 | W25Q128 raw driver | 完成：JEDEC ID、分页写、扇区擦除、超时和恢复路径已验收。 |
| M14 | littlefs and resource streaming | 完成：固定分区和图片/字体/文本分块读取已验收。 |
| M15 | USB diagnostic/log/resource protocol | 完成：MWRP 帧、完整性检查、有界路径、原子重命名和板端协议已闭合。 |
| M16 | KT6368 SPP transport | 完成：USART1 DMA+IDLE 桥接及故障恢复路径已验收。 |
| M17 | YModem candidate download and package verification | 完成：USB CDC 下载、包校验和 candidate-ready 持久化已验收。 |
| M18 | Backup, install, and power-loss recovery | 待板测：主机故障模型和非破坏性板端路径已验收；真实擦写边界断电恢复待 C1。 |
| M19 | Trial confirmation and rollback | 待板测：纯 C 状态模型和非破坏性板端健康确认路径已验收；主动 HardFault 板测待 C1。 |
| M20 | Final configurations, simulator, fault injection, budgets, regression report | 条件通过：本地矩阵已通过；最终状态等待 C1 的 M18/M19 板测和 C2 再验收。 |

## Current round: C0 documentation realignment

C0 aligns this page, the development plan, and README with implementation
baseline `origin/main=9bceb0e`. The following local revalidation passed:

```text
Debug/Diagnostic/Release build       pass
Debug format-check + cppcheck       pass
Host CTest                           21/21
240x280 UI simulator                 1/1
Signed manifest host tests           8/8
ELF/map origin and budget checks     pass
```

The host and configuration evidence above is the C0 record. This round does not
claim the unperformed destructive power-loss or active HardFault board tests.

## Next round: C1 M18c/M19b destructive board acceptance

C1 is pending the hardware gate: target VDD must be physically cut, USB VBUS
and ST-Link must not backfeed it, and both CDC and SWD must disappear during
the cut and reconnect after recovery. The test must use the existing Debug
symbols and state machine, with no permanent firmware test command or backdoor.

After C1 succeeds and merges, C2 M20b repeats the final matrix from the latest
`origin/main`, adds a higher-counter no-fault OTA and the required SPP soak,
and may close M20 only if every critical item passes.

## Risks and blockers

- The signed `security_counter` policy is persisted and compared; replay and
  rollback are covered by the M19 transition model and existing non-destructive
  evidence.
- M10 and M12 are intentionally closed as hardware-limited scope. The available
  board does not justify adding a board respin, battery, current target, or
  component-repair task.
- LIS2MDL is routed through the LSM6DS3 auxiliary Sensor Hub and remains a
  documented degraded hardware fact, not a software acceptance failure.
- The exact AHT20/AHT21 package marking, CW2015 population/routing, and EEPROM
  part/strap remain documented hardware risks.
- Editor regeneration remains a manual LVGL Pro Editor operation. CI builds
  committed generated C and does not replace the generator.
- CDC diagnostic output requires an active reader during acceptance; the C0
  host and configuration results do not replace the C1 board gate.
- Deliberate M18 erase/write power-loss and active M19 HardFault board tests are
  pending; they must be run only after the stated power-isolation gate is met.
- This remains a reference firmware, not a production secure-OTA product.

## Latest verification

- Debug App: `360,044 B` flash / `96,308 B` RAM; Diagnostic App: `369,584 B` /
  `96,308 B`; Release App: `195,440 B` / `96,296 B`.
- Debug/Diagnostic Bootloader: `18,664 B` flash / `1,208 B` RAM; Release
  Bootloader: `11,740 B` / `1,208 B` RAM. All are within the configured
  400 KiB App, 64 KiB Bootloader flash, and 128 KiB RAM budgets.
- Host CTest passed `21/21`, the 240x280 simulator passed `1/1`, and signed
  manifest tests passed `8/8`.
- No private key, device path, temporary image, dump, or complete serial log is
  part of this repository.
