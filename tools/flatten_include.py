#!/usr/bin/env python3
"""Flatten include/: public headers live at include/ root only."""

from __future__ import annotations

import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INC = ROOT / "include"

SUBDIRS = [
    INC / "bm" / "common",
    INC / "bm" / "core",
    INC / "bm" / "hybrid",
    INC / "bm" / "hal",
    INC / "bm" / "ultra",
    INC / "drv",
]

FORWARDER_MARK = "Forwarder"


def is_forwarder(path: Path) -> bool:
    try:
        text = path.read_text(encoding="utf-8")
    except OSError:
        return False
    return FORWARDER_MARK in text.splitlines()[0] if text else False


def main() -> None:
    removed = 0
    for path in sorted(INC.glob("*.h")):
        if is_forwarder(path):
            path.unlink()
            removed += 1

    moved = 0
    for sub in SUBDIRS:
        if not sub.is_dir():
            continue
        for src in sorted(sub.glob("*.h")):
            dst = INC / src.name
            if dst.exists():
                raise SystemExit(f"duplicate public header: {src.name}")
            shutil.move(str(src), str(dst))
            moved += 1

    zeplod_nested = INC / "zeplod" / "bm.h"
    if zeplod_nested.is_file():
        text = zeplod_nested.read_text(encoding="utf-8")
        text = text.replace("ZEPLOD_BM_H", "ZEPLOD_H")
        text = text.replace("@file bm.h", "@file zeplod.h")
        (INC / "zeplod.h").write_text(text, encoding="utf-8", newline="\n")

    for sub in [INC / "bm", INC / "drv", INC / "zeplod"]:
        if sub.is_dir():
            shutil.rmtree(sub)

    print(f"removed {removed} forwarders, moved {moved} headers -> {INC}")


if __name__ == "__main__":
    main()
