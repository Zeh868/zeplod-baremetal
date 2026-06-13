# sdk_esp32_idf — ESP32-WROOM-32E ESP-IDF 后端

基于 **ESP-IDF driver API**（`gptimer`、`uart`、`esp_task_wdt`），不再维护手写寄存器映射。

## ESP-IDF 工程集成（推荐）

1. 将 Zeplod 仓库加入 `EXTRA_COMPONENT_DIRS`，或把本目录复制为组件 `components/zeplod_esp32_backend`。
2. 在应用 `CMakeLists.txt` 中 `REQUIRES zeplod_esp32_backend`（或对应组件名）。
3. 链接 Zeplod 框架目标 `bm_framework` / `zeplod::framework`。

本目录含 `idf_component.yml`，要求 ESP-IDF ≥ 5.0。

## Zeplod 独立 CMake

```bash
export IDF_PATH=/path/to/esp-idf
cmake -B build -DBM_BACKEND=sdk_esp32_idf \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-xtensa-esp32.cmake
```

独立构建仅注入 ESP-IDF 头文件路径；**链接阶段仍需在 idf.py 工程中完成**。量产请使用上一节方式。
