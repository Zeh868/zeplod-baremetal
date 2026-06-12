# Demo 运行脚本

从仓库根目录或本目录调用均可；产物统一写入 **`build/demo/<variant>/<example>/`**，不污染 `Demo/` 源码树。

| 脚本 | 用途 |
|------|------|
| `run_native.ps1` | PC `native_sim` 构建与运行 |
| `run_qemu.sh` / `run_qemu.ps1` | QEMU 冒烟（CI 用 `.sh`） |
| `run_single.*` | 交互式构建并运行单个 QEMU 示例 |
| `run_all.*` | 批量构建并校验 `examples.txt` 列表 |
| `examples.txt` | 参与批量运行的示例名 |
| `qemu_example.mk` | 纯 Makefile 示例（如 `ultra_blink`）的共享规则 |

```powershell
.\tools\demo\run_native.ps1 core_sensor
.\tools\demo\run_qemu.ps1 interrupt_demo
```

```bash
./tools/demo/run_qemu.sh hrt_servo_stub
```

说明见 [docs/06-示例与上手路径.md](../../docs/06-示例与上手路径.md)。
