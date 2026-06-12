# NXP MCUXpresso 集成

与 STM32 思路相同：保留 SDK 的 `board/`、`drivers/`、`startup/`，Zeplod 作为额外源码组。

## CMake（mcuxpresso-sdk 风格）

```cmake
set(ZEPLOD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../zeplod-baremetal")
include(${ZEPLOD_ROOT}/cmake/zeplod.cmake)

zeplod_configure(
    ROOT    ${ZEPLOD_ROOT}
    PROFILE event
    BACKEND external
    CONFIG  ${CMAKE_CURRENT_SOURCE_DIR}/source/bm_config.h
)

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    source/bm_drv_mcux_glue.c
)
zeplod_link(${MCUX_SDK_PROJECT_NAME})
```

`bm_drv_mcux_glue.c` 内调用 `GPIO_PinWrite`、`LPUART_WriteBlocking`、`GPT_*` 等 SDK API，导出 `bm_drv_*_api` 符号表。

## MCUXpresso IDE（无 CMake）

```bash
python tools/list_sources.py --profile event --with-hal --format absolute \
    --root /path/to/zeplod-baremetal > zeplod_files.txt
```

将输出文件拖入 IDE 的 **Zeplod** 源组；Include 路径用 `--list-includes --format absolute`。

后端选 `external` 时，**不要**加入 `platform/backends/register_*`，只保留胶水 + HAL 分发层。

## 与 FlexPWM / ADC 混合域

参考 `platform/backends/register_stm32g4/bm_drv_adc_stm32g4.c` 的 `bind_complete` 模式：在 ADC 完成中断里调用绑定的 `bm_hal_hrt_binding_t.callback`。
