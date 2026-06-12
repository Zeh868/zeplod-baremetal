#!/usr/bin/env bash
set -euo pipefail

EXAMPLE="${1:?usage: run_qemu.sh <example>}"
# shellcheck source=demo_paths.sh
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/demo_paths.sh"

SRC="$DEMO_DIR/$EXAMPLE"
BUILD="$BUILD_DEMO_ROOT/qemu/$EXAMPLE"
TOOLCHAIN="$ROOT_DIR/cmake/toolchain-arm-none-eabi.cmake"
TIMEOUT="${QEMU_TIMEOUT_SEC:-60}"

if ! grep -qx "$EXAMPLE" "$EXAMPLES_LIST"; then
    echo "Unknown example: $EXAMPLE" >&2
    exit 1
fi

cmake -G 'Unix Makefiles' -S "$SRC" -B "$BUILD" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"
cmake --build "$BUILD"

OUT="$(mktemp)"
set +e
timeout "$TIMEOUT" qemu-system-arm -machine microbit -cpu cortex-m0 \
    -kernel "$BUILD/$EXAMPLE.elf" --semihosting -display none >"$OUT" 2>&1
RC=$?
set -e

cat "$OUT"

if [[ $RC -eq 124 ]]; then
    rm -f "$OUT"
    echo "QEMU timed out for $EXAMPLE" >&2
    exit 1
fi
grep -q ': PASS' "$OUT" || { rm -f "$OUT"; exit 1; }
rm -f "$OUT"
