#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD="${BUILD:-release}"
NEXTION_UI="${NEXTION_UI:-1}"
NEXTION_USE_LINA="${NEXTION_USE_LINA:-0}"
C2000_CGT_ROOT="${C2000_CGT_ROOT:-/home/gus/ti/c2000-cgt-18.12.1}"

case "$BUILD" in
    release|debug)
        ;;
    *)
        echo "BUILD must be release or debug, got: $BUILD" >&2
        exit 2
        ;;
esac

make -C "$PROJECT_DIR" -f "$SCRIPT_DIR/Makefile" clean "$BUILD" \
    BUILD="$BUILD" \
    NEXTION_UI="$NEXTION_UI" \
    NEXTION_USE_LINA="$NEXTION_USE_LINA" \
    C2000_CGT_ROOT="$C2000_CGT_ROOT"
