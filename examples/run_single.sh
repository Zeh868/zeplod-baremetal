#!/bin/bash
set -e

EXAMPLE="$1"
if [ -z "$EXAMPLE" ]; then
    echo "Usage: $0 <example_name>"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR/.."

EXAMPLE_DIR="$SCRIPT_DIR/$EXAMPLE"
if [ ! -d "$EXAMPLE_DIR" ]; then
    echo "Example '$EXAMPLE' not found"
    exit 1
fi

cd "$EXAMPLE_DIR"

echo "=== Building $EXAMPLE ==="
cmake -G "Unix Makefiles" -B build -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/cmake/toolchain-arm-none-eabi.cmake" . >/dev/null 2>&1
cmake --build build >/dev/null 2>&1

echo "=== Running $EXAMPLE in QEMU (interactive) ==="
echo "Press Ctrl+A then X to quit QEMU"
echo ""

qemu-system-arm -machine microbit -cpu cortex-m0 \
    -kernel "build/${EXAMPLE}.elf" --semihosting -nographic -serial stdio
