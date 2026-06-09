#!/usr/bin/env python3
"""list_sources.py — generate source file list for Keil/IAR/Make integration."""
import argparse
import sys

CORE_SRCS = [
    "src/core/bm_critical.c",
    "src/core/bm_event.c",
    "src/core/bm_mempool.c",
]

MODULE_SRCS = [
    "src/module/bm_module.c",
]

WDG_SRCS = [
    "src/core/bm_wdg.c",
]

HAL_QEMU_SRCS = [
    "hal_reference/qemu_cortex_m0/startup_qemu_cm0.s",
    "hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c",
]


def get_sources(enable_module: bool, enable_wdg: bool) -> list:
    srcs = list(CORE_SRCS)
    if enable_module:
        srcs.extend(MODULE_SRCS)
    if enable_wdg:
        srcs.extend(WDG_SRCS)
    srcs.extend(HAL_QEMU_SRCS)
    return srcs


def format_keil(paths: list, root: str) -> str:
    lines = []
    for p in paths:
        lines.append(f"../../{p}")
    return "\n".join(lines)


def format_iar(paths: list, root: str) -> str:
    # Same relative path format as Keil for source list
    return format_keil(paths, root)


def format_makefile(paths: list, root: str) -> str:
    lines = ["BM_SRCS := \\"]
    for p in paths:
        lines.append(f"    $(ZEPLOD_ROOT)/{p} \\")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Generate zeplod-baremetal source list")
    parser.add_argument("--enable-module", choices=["ON", "OFF"], default="ON")
    parser.add_argument("--enable-wdg", choices=["ON", "OFF"], default="OFF")
    parser.add_argument("--format", choices=["keil", "iar", "makefile"], default="keil")
    parser.add_argument("--root", default=".", help="Framework root path")
    args = parser.parse_args()

    enable_module = args.enable_module == "ON"
    enable_wdg = args.enable_wdg == "ON"
    paths = get_sources(enable_module, enable_wdg)

    fmt = args.format
    if fmt == "keil":
        out = format_keil(paths, args.root)
    elif fmt == "iar":
        out = format_iar(paths, args.root)
    elif fmt == "makefile":
        out = format_makefile(paths, args.root)
    else:
        out = "\n".join(paths)

    print(out)


if __name__ == "__main__":
    main()
