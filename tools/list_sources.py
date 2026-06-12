#!/usr/bin/env python3
"""Generate Zeplod source/include lists for CMake, Keil, IAR, and Makefile."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

HAL_DISPATCH = [
    "src/hal/bm_hal_critical.c",
    "src/hal/bm_hal_uart.c",
    "src/hal/bm_hal_timer.c",
    "src/hal/bm_hal_wdg.c",
    "src/hal/bm_hal_memory.c",
    "src/hal/bm_hal_pwm.c",
    "src/hal/bm_hal_adc.c",
    "src/hal/bm_hal_comp.c",
    "src/hal/bm_hal_encoder.c",
]

CORE = [
    "src/core/bm_critical.c",
    "src/core/bm_event.c",
    "src/core/bm_mempool.c",
    "src/core/bm_log.c",
    "src/core/bm_ultra.c",
]

COMPONENTS = {
    "module": ["src/core/bm_module.c"],
    "channel": ["src/core/bm_channel.c"],
    "shell": ["src/core/bm_shell.c"],
    "wdg": ["src/core/bm_wdg.c"],
    "hrt": ["src/hybrid/bm_hrt.c"],
    "ticker": ["src/hybrid/bm_ticker.c"],
    "resource": ["src/hybrid/bm_resource.c"],
    "ctrl_inst": ["src/hybrid/bm_ctrl_inst.c"],
    "sync": ["src/hybrid/bm_sync.c"],
}

BACKENDS: dict[str, list[str]] = {
    "native_sim": [
        "platform/backends/native_sim/bm_drv_singleton_native.c",
        "platform/backends/native_sim/bm_drv_pwm_native.c",
        "platform/backends/native_sim/bm_drv_adc_native.c",
        "platform/backends/native_sim/bm_drv_comp_native.c",
        "platform/backends/native_sim/bm_drv_encoder_native.c",
    ],
    "register_stm32g4": [
        "platform/backends/register_stm32g4/bm_drv_singleton_stm32g4.c",
        "platform/backends/register_stm32g4/bm_drv_pwm_stm32g4.c",
        "platform/backends/register_stm32g4/bm_drv_adc_stm32g4.c",
        "platform/backends/register_stm32g4/bm_drv_comp_stm32g4.c",
        "platform/backends/register_stm32g4/bm_drv_encoder_stm32g4.c",
        "platform/backends/register_stm32g4/bm_sync_hal_stm32g4.c",
    ],
    "register_esp32wroom32e": [
        "platform/backends/register_esp32wroom32e/bm_drv_singleton_esp32.c",
    ],
    "register_ch32v003": [
        "platform/backends/register_ch32v003/bm_drv_singleton_ch32v003.c",
    ],
    "qemu_cortex_m0": [
        "platform/backends/qemu_cortex_m0/bm_drv_singleton_qemu.c",
        "platform/backends/native_sim/bm_drv_pwm_native.c",
        "platform/backends/native_sim/bm_drv_adc_native.c",
        "platform/backends/native_sim/bm_drv_comp_native.c",
        "platform/backends/native_sim/bm_drv_encoder_native.c",
    ],
}

PROFILES: dict[str, dict[str, bool]] = {
    "minimal": {
        "module": False,
        "channel": False,
        "shell": False,
        "wdg": False,
        "hrt": False,
        "ticker": False,
        "ctrl_inst": False,
        "sync": False,
    },
    "event": {
        "module": True,
        "channel": False,
        "shell": False,
        "wdg": True,
        "hrt": False,
        "ticker": False,
        "ctrl_inst": False,
        "sync": False,
    },
    "servo": {
        "module": False,
        "channel": False,
        "shell": False,
        "wdg": False,
        "hrt": True,
        "ticker": True,
        "ctrl_inst": True,
        "sync": False,
    },
    "full": {
        "module": True,
        "channel": False,
        "shell": False,
        "wdg": True,
        "hrt": True,
        "ticker": True,
        "ctrl_inst": True,
        "sync": False,
    },
}

INCLUDE_APP = [
    "include/bm/common",
    "include/bm/core",
    "include/bm/hybrid",
    "include/bm/hal",
    "include/bm/ultra",
]

INCLUDE_DRV = ["include/drv"]


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
        paths.extend(INCLUDE_DRV)
    if args.backend and args.backend != "external":
        paths.append(f"platform/backends/{args.backend}")
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
    parser.add_argument(
        "--profile",
        choices=sorted(PROFILES),
        help="Preset component bundle (overrides individual --enable-*)",
    )
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
        help="Optional: also list reference port sources under platform/backends/ "
        "(for porting study). Production port: integration/port/bm_port.c",
    )
    parser.add_argument(
        "--with-hal",
        action="store_true",
        help="Include src/hal dispatch layer (required for hardware)",
    )
    parser.add_argument(
        "--list-includes",
        action="store_true",
        help="Output include paths instead of sources",
    )
    parser.add_argument(
        "--format",
        choices=[
            "relative",
            "absolute",
            "makefile",
            "cmake",
            "keil",
            "iar",
            "json",
        ],
        default="relative",
    )
    parser.add_argument("--root", default=".", help="Framework repository root")
    parser.add_argument(
        "--root-macro",
        default="ZEPLOD_ROOT",
        help="IDE root macro for keil/iar/cmake output",
    )
    args = parser.parse_args()

    if args.backend or args.with_hal:
        args.with_hal = True

    root = Path(args.root).resolve()
    if args.list_includes:
        paths = collect_includes(args)
    else:
        paths = collect_sources(args)

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
