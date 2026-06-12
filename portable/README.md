# portable（平台相关代码）

类比 FreeRTOS 的 `portable/` 目录。

```text
portable/
├── template/           # Port 模板（复制到应用工程）
│   └── bm_port.c
├── boot/               # 启动与链接脚本（QEMU 等）
│   └── qemu_cortex_m0/
├── native_sim/         # PC 参考 Port
├── register_stm32g4/   # STM32G4 寄存器参考 Port
├── register_esp32wroom32e/
├── register_ch32v003/
└── qemu_cortex_m0/     # QEMU 参考 Port
```

| 路径 | 用途 |
|------|------|
| `template/` | 量产：复制 `bm_port.c` 到 Cube/SDK 工程并改 HAL 调用 |
| `native_sim` 等 | 开发/测试：CMake `BM_BACKEND` 直接链接 |
| `boot/` | 仅 QEMU 固件；真机用厂商 startup |

集成说明：[docs/integration/README.md](../docs/integration/README.md)
