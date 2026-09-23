#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

TIMEOUT_SECONDS="${OPENMAC_TEST_TIMEOUT:-8}"
LOG_FILE="$(mktemp)"
DEBUG_LOG="zig-out/openmac-debug.log"
trap 'rm -f "$LOG_FILE" "$DEBUG_LOG"' EXIT

echo "[OpenMac test] Building UEFI kernel..."

if ! zig build; then
    echo "[FAIL] Kernel build failed."
    exit 1
fi

rm -f "$DEBUG_LOG"

echo "[OpenMac test] Booting through QEMU/OVMF..."
echo "[OpenMac test] Timeout: ${TIMEOUT_SECONDS}s"

set +e
timeout --signal=TERM --kill-after=2s "${TIMEOUT_SECONDS}s" zig build run-uefi >"$LOG_FILE" 2>&1
QEMU_STATUS=$?
set -e

cat "$LOG_FILE"

if [ "$QEMU_STATUS" -ne 124 ]; then
    echo
    echo "[FAIL] QEMU did not remain running until the test timeout."
    echo "[FAIL] Exit status: $QEMU_STATUS"
    exit 1
fi

if ! grep -Fq "UEFI:entered" "$LOG_FILE"; then
    echo
    echo "[FAIL] UEFI application entry point was not observed on the serial console."
    echo "[FAIL] This indicates the failure is before main() or before serial initialization."
    exit 1
fi

if [ ! -f "$DEBUG_LOG" ]; then
    echo
    echo "[FAIL] QEMU produced no kernel debug log after UEFI entry."
    exit 1
fi

echo
echo "[OpenMac test] Kernel debug trace:"
cat "$DEBUG_LOG"

required_debug_messages=(
    "KERNEL:entered"
    "KERNEL:serial-init"
    "KERNEL:idle"
)

for message in "${required_debug_messages[@]}"; do
    if ! grep -Fq "$message" "$DEBUG_LOG"; then
        echo
        echo "[FAIL] Missing kernel stage: $message"
        exit 1
    fi
done

required_messages=(
    "OpenMac kernel entered."
    "Boot services are no longer available."
    "Memory map bytes:"
    "Descriptor size:"
    "Kernel idle loop reached."
)

for message in "${required_messages[@]}"; do
    if ! grep -Fq "$message" "$LOG_FILE"; then
        echo
        echo "[FAIL] Missing serial output: $message"
        exit 1
    fi
done

echo
echo "[PASS] OpenMac kernel boot test passed."
