# 19 IAR 集成

> **本文职责**：在 IAR EWARM 中添加 Zeplod 源文件、头文件路径与 Port。  
> **集成总览** → [13-集成到现有工程](13-集成到现有工程.md)。

## 1. 模型

Port 在应用工程；库以源码或 `.a` 形式加入（同 FreeRTOS 集成习惯）。

## 2. 源文件与 Include

```bash
python tools/list_sources.py --profile event --format iar --root-macro ZEPLOD_ROOT
python tools/list_sources.py --profile event --list-includes --format iar --root-macro ZEPLOD_ROOT
```

将 `portable/template/bm_port.c` 复制并改编后**单独加入**工程（不在 list_sources 输出中）。

Include 路径规则同 [18-Keil集成](18-Keil集成.md) §2。

## 3. 参考 Port

`portable/register_stm32g4/bm_drv_singleton_stm32g4.c` 展示了 STM32G4 上单例驱动表的接法。

静态库链接见 [15-静态库构建](15-静态库构建.md)。
