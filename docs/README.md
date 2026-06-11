# Zeplod Baremetal 文档

本目录提供**上手使用**与**技术原理**说明。建议按编号顺序阅读。

## 导读（推荐阅读顺序）

| 文档 | 内容 |
|------|------|
| [00-快速开始](00-快速开始.md) | 环境、首次构建、运行示例与单元测试 |
| [01-框架概览与资源层级](01-框架概览与资源层级.md) | 定位、组件地图、Ultra→Control 裁剪 |
| [02-混合域与执行模型](02-混合域与执行模型.md) | HRT / SRT 隔离、快照、同步域原理 |
| [03-核心机制](03-核心机制.md) | 事件、内存池、模块、通道、看门狗 |
| [04-构建配置与CMake](04-构建配置与CMake.md) | `bm_config.h`、CMake 选项与链接目标 |
| [05-示例与上手路径](05-示例与上手路径.md) | 示例递进路线与典型主循环结构 |
| [06-HAL移植指南](06-HAL移植指南.md) | 契约、参考实现、PWM/ADC、移植清单 |
| [07-测试与调试](07-测试与调试.md) | 单元测试、QEMU 冒烟、日志 |
| [08-迁移与演进](08-迁移与演进.md) | Ultra→Core→混合域→Zephyr（含 API 对照） |
| [09-安全与可靠性](09-安全与可靠性.md) | fail-stop、SIL 准备、需求—测试追溯 |

## 专题与附录

| 路径 | 说明 |
|------|------|
| [architecture.md](architecture.md) | 英文架构摘要（依赖方向、CMake 目标） |
| [api/](api/) | 混合域与 HAL 相关 API 参考 |
| [porting/](porting/) | Keil / IAR 工具链集成附录 |
| [reports/](reports/) | 硬件验证报告 |

## 仓库外入口

- 中文总览：[README.zh-CN.md](../README.zh-CN.md)
- 示例说明：[examples/README.md](../examples/README.md)
- 示例移植：[examples/PORTING.md](../examples/PORTING.md)

> `docs/superpowers/` 为本地 AI 辅助设计草稿，**不纳入版本控制**。
