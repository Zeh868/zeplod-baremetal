# Demo/

Zeplod 示例源码（类比 FreeRTOS/Demo/）。**仅含应用代码与 `CMakeLists.txt`**，不含构建产物。

- 完整说明：[docs/06-示例与上手路径.md](../docs/06-示例与上手路径.md)
- 运行脚本：[tools/demo/](../tools/demo/)

```powershell
.\tools\demo\run_native.ps1 core_sensor
.\tools\demo\run_qemu.ps1 interrupt_demo
```

构建产物统一在仓库根目录 `build/demo/`（见 [tools/demo/README.md](../tools/demo/README.md)）。
