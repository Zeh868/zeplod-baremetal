#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_ROOT="$SCRIPT_DIR/build/unix"
TOOLCHAIN="$ROOT_DIR/cmake/toolchain-arm-none-eabi.cmake"
FAILED=()

while IFS= read -r example; do
    [[ -z "$example" || "$example" == \#* ]] && continue
    source_dir="$SCRIPT_DIR/$example"
    build_dir="$BUILD_ROOT/$example"

    echo "=== Building $example ==="
    cmake -G "Unix Makefiles" -S "$source_dir" -B "$build_dir" \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"
    cmake --build "$build_dir"

    echo "=== Running $example in QEMU ==="
    log_file="$build_dir/qemu.log"
    timeout 8s qemu-system-arm -machine microbit -cpu cortex-m0 \
        -kernel "$build_dir/$example.elf" --semihosting -display none \
        >"$log_file" 2>&1 || true
    head -n 30 "$log_file"

    if grep -q "EXAMPLE_.*: PASS" "$log_file"; then
        echo "$example ... PASS"
    else
        echo "$example ... FAIL"
        FAILED+=("$example")
    fi
done < "$SCRIPT_DIR/examples.txt"

if ((${#FAILED[@]} > 0)); then
    echo "Failed examples: ${FAILED[*]}"
    exit 1
fi

echo "All examples passed."
