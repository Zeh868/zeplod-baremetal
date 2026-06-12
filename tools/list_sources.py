#!/usr/bin/env python3
"""Generate Zeplod Source/include file lists for CMake, Keil, IAR, and Makefile."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

HAL_DISPATCH = [
    "Source/hal/bm_hal_critical.c",
    "Source/hal/bm_hal_uart.c",
    "Source/hal/bm_hal_timer.c",
    "Source/hal/bm_hal_wdg.c",
    "Source/hal/bm_hal_memory.c",
    "Source/hal/bm_hal_pwm.c",
    "Source/hal/bm_hal_adc.c",
    "Source/hal/bm_hal_comp.c",
    "Source/hal/bm_hal_encoder.c",
]

CORE = [
    "Source/core/bm_critical.c",
    "Source/core/bm_event.c",
    "Source/core/bm_mempool.c",
    "Source/core/bm_log.c",
    "Source/core/bm_ultra.c",
]

COMPONENTS = {
    "module": ["Source/core/bm_module.c"],
    "channel": ["Source/core/bm_channel.c"],
    "shell": ["Source/core/bm_shell.c"],
    "wdg": ["Source/core/bm_wdg.c"],
    "hrt": ["Source/hybrid/bm_hrt.c"],
    "ticker": ["Source/hybrid/bm_ticker.c"],
    "resource": ["Source/hybrid/bm_resource.c"],
    "ctrl_inst": ["Source/hybrid/bm_ctrl_inst.c"],
    "sync": ["Source/hybrid/bm_sync.c"],
}

BACKENDS: dict[str, list[str]] = {
    "native_sim": [
        "portable/native_sim/bm_drv_singleton_native.c",
        "portable/native_sim/bm_drv_pwm_native.c",
        "portable/native_sim/bm_drv_adc_native.c",
        "portable/native_sim/bm_drv_comp_native.c",
        "portable/native_sim/bm_drv_encoder_native.c",
    ],
    "register_stm32g4": [
        "portable/register_stm32g4/bm_drv_singleton_stm32g4.c",
        "portable/register_stm32g4/bm_drv_pwm_stm32g4.c",
        "portable/register_stm32g4/bm_drv_adc_stm32g4.c",
        "portable/register_stm32g4/bm_drv_comp_stm32g4.c",
        "portable/register_stm32g4/bm_drv_encoder_stm32g4.c",
        "portable/register_stm32g4/bm_sync_hal_stm32g4.c",
    ],
    "register_esp32wroom32e": [
        "portable/register_esp32wroom32e/bm_drv_singleton_esp32.c",
    ],
    "register_ch32v003": [
        "portable/register_ch32v003/bm_drv_singleton_ch32v003.c",
    ],
    "qemu_cortex_m0": [
        "portable/qemu_cortex_m0/bm_drv_singleton_qemu.c",
        "portable/native_sim/bm_drv_pwm_native.c",
        "portable/native_sim/bm_drv_adc_native.c",
        "portable/native_sim/bm_drv_comp_native.c",
        "portable/native_sim/bm_drv_encoder_native.c",
    ],
}

PROFILES: dict[str, dict[str, bool]] = {
    "minimal": {
        "module": False, "channel": False, "shell": False, "wdg": False,
        "hrt": False, "ticker": False, "ctrl_inst": False, "sync": False,
    },
    "event": {
        "module": True, "channel": False, "shell": False, "wdg": True,
        "hrt": False, "ticker": False, "ctrl_inst": False, "sync": False,
    },
    "servo": {
        "module": False, "channel": False, "shell": False, "wdg": False,
        "hrt": True, "ticker": True, "ctrl_inst": True, "sync": False,
    },
    "full": {
        "module": True, "channel": False, "shell": False, "wdg": True,
        "hrt": True, "ticker": True, "ctrl_inst": True, "sync": False,
    },
}

INCLUDE_APP = ["include"]

INCLUDE_PORT_EXTRA = [
    "include/bm/common",
    "include/bm/core",
    "include/bm/hybrid",
    "include/hal",
    "include/drv",
]


def enabled(value: str) -> bool:
    return value == "ON"


def profile_flags(args: argparse.Namespace) -> dict[str, bool]:
    if args.profile:
        if args.profile not in PROFILES:
            raise SystemExit(f"Unknown profile: {args.profile}")
        return dict(PROFILES[args.profile])
    return {
        "module": enabled(args.enable_module),
        "channel": enabled(args.enable_channel),
        "shell": enabled(args.enable_shell),
        "wdg": enabled(args.enable_wdg),
        "hrt": enabled(args.enable_hrt),
        "ticker": enabled(args.enable_ticker),
        "ctrl_inst": enabled(args.enable_ctrl_inst),
        "sync": enabled(args.enable_sync),
    }


def collect_sources(args: argparse.Namespace) -> list[str]:
    flags = profile_flags(args)
    sources = list(CORE)
    for name, on in flags.items():
        if on:
            sources.extend(COMPONENTS[name])
    if args.with_hal:
        sources.extend(HAL_DISPATCH)
    if args.backend and args.backend != "external":
        if args.backend not in BACKENDS:
            raise SystemExit(
                f"Unknown backend: {args.backend}. "
                f"Choices: {', '.join(sorted(BACKENDS))}, external"
            )
        sources.extend(BACKENDS[args.backend])
    return sources


def collect_includes(args: argparse.Namespace) -> list[str]:
    paths = list(INCLUDE_APP)
    if args.with_hal or args.backend:
        paths.extend(INCLUDE_PORT_EXTRA)
    if args.backend and args.backend != "external":
        paths.append(f"portable/{args.backend}")
    return paths


def resolve_paths(paths: list[str], root: Path) -> list[str]:
    return [str((root / path).resolve()) for path in paths]


def format_plain(paths: list[str]) -> str:
    return "\n".join(paths)


def format_makefile(paths: list[str], macro: str) -> str:
    lines = [f"{macro} := \\"]
    for index, path in enumerate(paths):
        suffix = " \\" if index < len(paths) - 1 else ""
        lines.append(f"    $(ZEPLOD_ROOT)/{path}{suffix}")
    return "\n".join(lines)


def format_cmake_list(name: str, paths: list[str]) -> str:
    lines = [f"set({name}"]
    for path in paths:
        lines.append(f'    "${{ZEPLOD_ROOT}}/{path}"')
    lines.append(")")
    return "\n".join(lines)


def format_ide(paths: list[str], root_var: str) -> str:
    return "\n".join(f"{root_var}/{path}" for path in paths)


def format_json_payload(args: argparse.Namespace, root: Path) -> str:
    payload = {
        "profile": args.profile or "custom",
        "backend": args.backend,
        "includes": collect_includes(args),
        "sources": collect_sources(args),
    }
    return json.dumps(payload, indent=2)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate Zeplod integration file lists for any IDE"
    )
    parser.add_argument("--profile", choices=sorted(PROFILES))
    parser.add_argument("--enable-module", choices=["ON", "OFF"], default="ON")
    parser.add_argument("--enable-channel", choices=["ON", "OFF"], default="OFF")
    parser.add_argument("--enable-shell", choices=["ON", "OFF"], default="OFF")
    parser.add_argument("--enable-wdg", choices=["ON", "OFF"], default="OFF")
    parser.add_argument("--enable-hrt", choices=["ON", "OFF"], default="OFF")
    parser.add_argument("--enable-ticker", choices=["ON", "OFF"], default="OFF")
    parser.add_argument("--enable-ctrl-inst", choices=["ON", "OFF"], default="OFF")
    parser.add_argument("--enable-sync", choices=["ON", "OFF"], default="OFF")
    parser.add_argument(
        "--backend",
        help="Optional reference port under portable/ (study only). "
        "Production: portable/template/bm_port.c in app project.",
    )
    parser.add_argument("--with-hal", action="store_true")
    parser.add_argument("--list-includes", action="store_true")
    parser.add_argument(
        "--format",
        choices=["relative", "absolute", "makefile", "cmake", "keil", "iar", "json"],
        default="relative",
    )
    parser.add_argument("--root", default=".")
    parser.add_argument("--root-macro", default="ZEPLOD_ROOT")
    args = parser.parse_args()

    if args.backend or args.with_hal:
        args.with_hal = True

    root = Path(args.root).resolve()
    paths = collect_includes(args) if args.list_includes else collect_sources(args)

    if args.format == "absolute":
        output = format_plain(resolve_paths(paths, root))
    elif args.format == "makefile":
        macro = "ZEPLOD_INCLUDES" if args.list_includes else "ZEPLOD_SRCS"
        output = format_makefile(paths, macro)
    elif args.format == "cmake":
        name = "ZEPLOD_INCLUDE_DIRS" if args.list_includes else "ZEPLOD_SOURCES"
        output = format_cmake_list(name, paths)
    elif args.format in ("keil", "iar"):
        output = format_ide(paths, f"$({args.root_macro})")
    elif args.format == "json":
        output = format_json_payload(args, root)
    else:
        output = format_plain(paths)

    print(output)


if __name__ == "__main__":
    main()
