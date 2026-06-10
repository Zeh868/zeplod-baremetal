#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

EXAMPLES=(ultra_blink core_sensor full_system interrupt_demo)
FAILED=()

for ex in "${EXAMPLES[@]}"; do
    echo "=== Building $ex ==="
    cd "$ex"
    cmake -G "Unix Makefiles" -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake . >/dev/null 2>&1
    cmake --build build >/dev/null 2>&1

    echo "=== Running $ex in QEMU ==="
    timeout 10s qemu-system-arm -machine microbit -cpu cortex-m0 \
        -kernel "build/${ex}.elf" --semihosting -nographic -serial stdio > /tmp/${ex}.log 2>&1 || true

    if grep -q "EXAMPLE_.*: PASS" /tmp/${ex}.log; then
        echo "$ex ... PASS"
    else
        echo "$ex ... FAIL"
        FAILED+=("$ex")
    fi
    cd ..
done

if [ ${#FAILED[@]} -eq 0 ]; then
    echo "All examples passed."
    exit 0
else
    echo "Failed: ${FAILED[*]}"
    exit 1
fi
