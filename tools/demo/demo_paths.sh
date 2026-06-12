#!/usr/bin/env bash
# Shared paths for Demo run scripts. Source from tools/demo/*.sh
DEMO_TOOLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$DEMO_TOOLS_DIR/../.." && pwd)"
DEMO_DIR="$ROOT_DIR/Demo"
BUILD_DEMO_ROOT="$ROOT_DIR/build/demo"
EXAMPLES_LIST="$DEMO_TOOLS_DIR/examples.txt"
