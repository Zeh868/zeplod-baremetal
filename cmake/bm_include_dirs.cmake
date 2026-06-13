# Zeplod Baremetal 头文件路径
# 应用只需 include/（对外 API 在根目录）；库编译额外搜索子目录
set(BM_INCLUDE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../include)

set(BM_INCLUDE_PUBLIC_DIRS ${BM_INCLUDE_ROOT})

set(BM_INCLUDE_INTERNAL_DIRS
    ${BM_INCLUDE_ROOT}/bm/common
    ${BM_INCLUDE_ROOT}/bm/core
    ${BM_INCLUDE_ROOT}/bm/hybrid
    ${BM_INCLUDE_ROOT}/bm/algorithm
    ${BM_INCLUDE_ROOT}/hal
    ${BM_INCLUDE_ROOT}/drv
)

set(BM_INCLUDE_APP_DIRS ${BM_INCLUDE_PUBLIC_DIRS})
set(BM_INCLUDE_BACKEND_DIRS ${BM_INCLUDE_PUBLIC_DIRS})

function(bm_target_internal_includes target)
    target_include_directories(${target} PRIVATE ${BM_INCLUDE_INTERNAL_DIRS})
endfunction()

# QEMU Cortex-M0 引导（启动汇编 / crt0 / 链接脚本）
set(BM_BOOT_QEMU_CM0_DIR ${CMAKE_CURRENT_LIST_DIR}/../portable/boot/qemu_cortex_m0)
