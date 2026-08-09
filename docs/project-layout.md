# 项目目录与依赖边界

本仓库是一个工作流实验仓库，当前包含 F411 手表工程、ESP32-S3 点灯工程和本地学习材料。每个板级工程保持独立构建，不建立一个强行统管所有芯片的根 CMake 工程。

## 当前目录

```text
.
├── firmware/
│   ├── stm32/f411_watch/
│   │   ├── Core/                 # CubeMX 生成的启动、HAL 初始化和中断代码
│   │   ├── Drivers/              # STM32 CMSIS/HAL
│   │   ├── Middlewares/          # FreeRTOS、FatFs、USB Device
│   │   ├── FATFS/ USB_DEVICE/    # CubeMX 集成代码
│   │   ├── cmake/stm32cubemx/    # CubeMX 生成的构建描述
│   │   ├── user/                 # 手写的 F411 代码
│   │   │   ├── app/              # 启动和组合，不放硬件寄存器操作
│   │   │   ├── board/            # F411 板级适配和显示/电源/传感器入口
│   │   │   └── config/           # 板级编译配置
│   │   └── docs/                 # F411 原理图和架构图
│   └── espressif/esp32s3_board/  # 独立的 ESP-IDF 工程
├── products/
│   └── f411_watch/
│       ├── core/                 # 纯 C 核心，与 F411 和主机测试共用
│       ├── runtime/              # 纯 C 时间、服务队列和任务健康契约
│       ├── sensors/              # 纯 C 传感器协议、采样和服务模型
│       ├── ui/                   # LVGL Pro Editor XML 和生成 C
│       └── simulator/            # 独立 PC CMake/CTest 消费者
├── docs/                         # 仓库级工作流和目录说明
├── tools/                        # 构建/烧录辅助脚本
├── .vscode/                      # 本仓库的构建和调试入口
└── learn/                        # 本地学习材料，不上传 Git
```

## CubeMX 边界

`Core/`、`Drivers/`、`Middlewares/`、`FATFS/`、`USB_DEVICE/`、`startup_stm32f411xe.s`、链接脚本以及 `cmake/stm32cubemx/CMakeLists.txt` 属于 CubeMX 生成或维护范围。除 `USER CODE BEGIN/END` 区域外不手工编辑这些文件。

F411 顶层 `CMakeLists.txt` 是 CubeMX 明确允许用户维护的入口。手写代码通过 `user/CMakeLists.txt` 的 `f411_watch_user` target 接入，避免把源码列表写回生成的 CMake 文件。

CubeMX 目前强制保留一个 `defaultTask`。它只作为启动壳存在，不承载手表业务；未来的实际任务、队列和互斥锁由手写运行时创建，不能把 CubeMX 的任务名称当作架构约束。

## 依赖方向

```text
CubeMX main USER CODE
        |
        +--> board/power
        |
        +--> app/watch_app --> board/display --> STM32 HAL
```

- 应用启动层只组合功能，不直接访问 HAL、GPIO、SPI 句柄或芯片寄存器。
- 板级代码可以依赖 CubeMX 生成的 HAL 句柄和引脚定义。
- 未来的设备驱动应隐藏总线寄存器细节，但不伪装成脱离具体器件语义的万能 API。
- LVGL 只能由唯一的 UI 任务拥有；业务核心、服务和驱动不能直接创建或操作 LVGL 对象。
- I2C、SPI 等共享总线的锁应位于平台/服务边界，不能散落在业务页面中。

## 按需扩展

只在出现真实实现需求后按产品边界建立目录。当前 M7 已有 UI 和 simulator
真实消费者；后续页面和服务仍按独立闭环增加：

```text
products/f411_watch/
├── core/        # 纯 C 状态机和跨平台契约
├── input/       # 纯 C 输入归一化和消抖契约
├── runtime/     # 纯 C 时间、服务队列和任务健康契约
├── sensors/     # 纯 C 传感器协议、采样和服务模型
├── ui/          # Editor XML 和提交的生成 C
└── simulator/   # 独立 PC CMake 工程与 CTest smoke
```

F411 的 `services/`、`drivers/`、`platform/` 同样按实际功能逐个建立。旧工程驱动只作为协议和硬件行为参考，不能整体复制旧的全局依赖和初始化架构。
