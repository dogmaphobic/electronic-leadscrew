#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD="${BUILD:-release}"
CCS_ROOT="${CCS_ROOT:-/home/gus/ti/ccs2100/ccs}"
DSLITE="${DSLITE:-$CCS_ROOT/ccs_base/DebugServer/bin/DSLite}"
TARGET_CONFIG="${TARGET_CONFIG:-$PROJECT_DIR/targetConfigs/TMS320F280049C.ccxml}"
OUT_FILE="${OUT_FILE:-$PROJECT_DIR/build/c2000/$BUILD/els-f280049c.out}"
LOG_FILE="${LOG_FILE:-$PROJECT_DIR/build/c2000/$BUILD/dslite-flash.log}"

if [[ ! -x "$DSLITE" ]]; then
    echo "DSLite not found or not executable: $DSLITE" >&2
    exit 2
fi

if [[ ! -f "$TARGET_CONFIG" ]]; then
    echo "Target config not found: $TARGET_CONFIG" >&2
    exit 2
fi

if [[ ! -f "$OUT_FILE" ]]; then
    echo "Firmware image not found: $OUT_FILE" >&2
    echo "Run ./build.sh first, or set OUT_FILE to the image to flash." >&2
    exit 2
fi

sudo "$DSLITE" flash \
    --config="$TARGET_CONFIG" \
    --core=0 \
    --before=Erase \
    --flash \
    --verify \
    --run \
    --verbose \
    --log="$LOG_FILE" \
    "$OUT_FILE"
