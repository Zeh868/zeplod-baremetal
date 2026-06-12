# Zeplod Baremetal — 公共头文件均在 include/ 根目录
set(BM_INCLUDE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../include)
set(BM_INCLUDE_APP_DIRS ${BM_INCLUDE_ROOT})
set(BM_INCLUDE_BACKEND_DIRS ${BM_INCLUDE_ROOT})

# QEMU Cortex-M0 引导（启动汇编 / crt0 / 链接脚本）
set(BM_BOOT_QEMU_CM0_DIR ${CMAKE_CURRENT_LIST_DIR}/../portable/boot/qemu_cortex_m0)
