#!/usr/bin/env bash
set -euo pipefail

if (($# != 1)); then
    echo "Usage: $0 <example_name>"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
EXAMPLE="$1"
SOURCE_DIR="$SCRIPT_DIR/$EXAMPLE"
BUILD_DIR="$SCRIPT_DIR/build/unix/$EXAMPLE"

if ! grep -Fxq "$EXAMPLE" "$SCRIPT_DIR/examples.txt" || [[ ! -d "$SOURCE_DIR" ]]; then
    echo "Unknown example '$EXAMPLE'"
    exit 1
fi

cmake -G "Unix Makefiles" -S "$SOURCE_DIR" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/cmake/toolchain-arm-none-eabi.cmake"
cmake --build "$BUILD_DIR"

echo "Press Ctrl+C to stop."
qemu-system-arm -machine microbit -cpu cortex-m0 \
    -kernel "$BUILD_DIR/$EXAMPLE.elf" --semihosting -display none
