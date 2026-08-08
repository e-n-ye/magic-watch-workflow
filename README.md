# Magic Watch Workflow

A CMake-first embedded watch workflow for STM32F411 and ESP32-S3, with reproducible builds, architecture diagrams, and incremental validation.

本仓库首先用于跑通嵌入式开发工作流：CMake + Ninja、板级工程隔离、Git 小提交、架构文档，以及后续的主机测试和 CI。它不是最终的双芯片手表产品仓库。

## Current status

- STM32F411 工程已经通过 CMake/Ninja 编译，并可使用 ST-Link 烧录。
- F411 当前验收目标是 LCD 背光和固定彩条显示。
- ESP32-S3 工程已经通过 ESP-IDF 点亮板载 RGB LED，并保留 UART 监视入口。
- CubeMX 生成区与手写 `user/` 代码已经分开；CubeMX 强制保留的 `defaultTask` 不承载手表业务。
- LVGL、XML UI、主机模拟器和状态机将在版本兼容性与真实需求明确后逐步加入。
- [F411 rolling development status](docs/f411-development-status.md)

## Documentation

### Hardware

- [F411 schematic / PCB design reference PDF](<firmware/stm32/f411_watch/docs/hardware/my_watch原理图_V2.0.pdf>)
- [Hardware block diagram source](firmware/stm32/f411_watch/docs/hardware/hardware-block-diagram.drawio)
- [Hardware system diagram](firmware/stm32/f411_watch/docs/hardware/hardware-block-diagram-系统结构图.drawio.png)
- [Power tree diagram](firmware/stm32/f411_watch/docs/hardware/hardware-block-diagram-电源树.drawio.png)

![F411 hardware system](firmware/stm32/f411_watch/docs/hardware/hardware-block-diagram-系统结构图.drawio.png)

### Software

- [Software architecture source](firmware/stm32/f411_watch/docs/architecture/software-architecture.drawio)
- [Layered architecture](firmware/stm32/f411_watch/docs/architecture/software-architecture-分层架构.png)
- [Runtime tasks and events](firmware/stm32/f411_watch/docs/architecture/software-architecture-任务与事件.png)
- [Project layout and dependency rules](docs/project-layout.md)

![Layered software architecture](firmware/stm32/f411_watch/docs/architecture/software-architecture-分层架构.png)

### Coding and formatting

- [C/C++ coding guidelines](docs/coding-guidelines.md)
- The root `.clang-format` is a project-level configuration for hand-written code.
- Automatic formatting is limited to the F411 `user/` whitelist; CubeMX-generated and third-party code is excluded.

## Build

### STM32F411

Open `firmware/stm32/f411_watch` in a terminal or use the VS Code tasks:

```sh
cmake --preset Debug
cmake --build --preset Debug
```

The generated App ELF is written to `build/Debug/my_watch_f411.elf`; the standalone Bootloader ELF is written to `build/Debug/f411_bootloader.elf`. VS Code provides build, OpenOCD/ST-Link, and STM32CubeProgrammer tasks. The CubeProgrammer task accepts `STM32_PROGRAMMER_CLI` as an override and otherwise checks PATH.

### ESP32-S3

Set `IDF_TOOLS_PATH` when ESP-IDF is not installed in the default local location, then run the VS Code ESP32-S3 build or flash task. The serial port is requested by the task instead of being fixed in the repository.

## CI

The initial GitHub Actions workflow protects only the F411 project. Pull requests, pushes to `main`, and manual runs classify changed paths first. F411 firmware, its Dockerfile, the formatting policy, or the workflow runs a pinned-image format-check, Debug build, and Cppcheck target; documentation-only changes skip the board job but still finish with `CI / CI Gate`.

The repository variable `F411_CI_IMAGE` must contain the Docker Hub image reference with its immutable digest, for example `docker.io/enen001/magic-watch-f411-ci@sha256:<digest>`. The workflow intentionally rejects `latest` and tag-only references.

## Working rules

- Read [CONTRIBUTING.md](CONTRIBUTING.md) before contributing. AI coding agents should also read [AGENTS.md](AGENTS.md).
- The required delivery flow is `branch -> code change -> Build + Cppcheck -> commit -> push -> pull request -> CI Gate -> Rebase and merge or rework`.
- Keep generated CubeMX files inside their generated boundary. Add hand-written sources through the top-level CMake entry and `user/CMakeLists.txt`.
- Make one focused change per commit.
- A commit should build the affected board target.
- Use the commit format `<type>:<scope>:<description>`, for example:
  `build:f411:connect hand-written user target`
  `docs:hardware:add f411 block diagrams`
  `fix:display:preserve power latch before gpio init`
- `learn/` is local study material and is intentionally excluded from Git.

## License

Original project code and documentation are released under the MIT License. STM32 vendor code, FreeRTOS, FatFs, USB middleware, ESP-IDF components, and other third-party materials retain their own licenses; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and the license files in their directories.
