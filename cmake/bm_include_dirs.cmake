# Zeplod Baremetal 头文件搜索路径（保持 #include "bm_*.h" 扁平写法）
set(BM_INCLUDE_COMMON   ${CMAKE_CURRENT_LIST_DIR}/../include/bm/common)
set(BM_INCLUDE_CORE     ${CMAKE_CURRENT_LIST_DIR}/../include/bm/core)
set(BM_INCLUDE_HYBRID   ${CMAKE_CURRENT_LIST_DIR}/../include/bm/hybrid)
set(BM_INCLUDE_HAL      ${CMAKE_CURRENT_LIST_DIR}/../include/bm/hal)
set(BM_INCLUDE_ULTRA    ${CMAKE_CURRENT_LIST_DIR}/../include/bm/ultra)
set(BM_INCLUDE_DRV      ${CMAKE_CURRENT_LIST_DIR}/../include/drv)

# 应用可见路径
set(BM_INCLUDE_APP_DIRS
    ${BM_INCLUDE_COMMON}
    ${BM_INCLUDE_CORE}
    ${BM_INCLUDE_HYBRID}
    ${BM_INCLUDE_HAL}
    ${BM_INCLUDE_ULTRA}
)

# 后端实现额外需要 drv/
set(BM_INCLUDE_BACKEND_DIRS
    ${BM_INCLUDE_APP_DIRS}
    ${BM_INCLUDE_DRV}
)

# QEMU Cortex-M0 引导（启动汇编 / crt0 / 链接脚本）
set(BM_BOOT_QEMU_CM0_DIR ${CMAKE_CURRENT_LIST_DIR}/../platform/boot/qemu_cortex_m0)
