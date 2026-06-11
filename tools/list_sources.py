#!/usr/bin/env python3
"""Generate framework-only source lists for IDE and Make integrations."""

import argparse
from pathlib import Path


COMPONENTS = {
    "core": [
        "src/core/bm_critical.c",
        "src/core/bm_event.c",
        "src/core/bm_mempool.c",
        "src/core/bm_log.c",
    ],
    "module": ["src/core/bm_module.c"],
    "channel": ["src/core/bm_channel.c"],
    "shell": ["src/core/bm_shell.c"],
    "wdg": ["src/core/bm_wdg.c"],
}


def enabled(value: str) -> bool:
    return value == "ON"


def get_sources(args: argparse.Namespace) -> list[str]:
    sources = list(COMPONENTS["core"])
    for name in ("module", "channel", "shell", "wdg"):
        if enabled(getattr(args, f"enable_{name}")):
            sources.extend(COMPONENTS[name])
    return sources


def resolve_paths(paths: list[str], root: str) -> list[str]:
    base = Path(root).resolve()
    return [str((base / path).resolve()) for path in paths]


def format_plain(paths: list[str]) -> str:
    return "\n".join(paths)


def format_makefile(paths: list[str]) -> str:
    lines = ["BM_SRCS := \\"]
    for index, path in enumerate(paths):
        suffix = " \\" if index < len(paths) - 1 else ""
        lines.append(f"    $(ZEPLOD_ROOT)/{path}{suffix}")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate zeplod-baremetal framework source lists"
    )
    parser.add_argument("--enable-module", choices=["ON", "OFF"], default="ON")
    parser.add_argument("--enable-channel", choices=["ON", "OFF"], default="OFF")
    parser.add_argument("--enable-shell", choices=["ON", "OFF"], default="OFF")
    parser.add_argument("--enable-wdg", choices=["ON", "OFF"], default="OFF")
    parser.add_argument(
        "--format",
        choices=["relative", "absolute", "keil", "iar", "makefile"],
        default="relative",
    )
    parser.add_argument("--root", default=".", help="Framework repository root")
    args = parser.parse_args()

    sources = get_sources(args)
    if args.format == "absolute":
        output = format_plain(resolve_paths(sources, args.root))
    elif args.format == "makefile":
        output = format_makefile(sources)
    else:
        output = format_plain(sources)

    print(output)


if __name__ == "__main__":
    main()
