#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

TIMEOUT_SECONDS="${OPENMAC_TEST_TIMEOUT:-8}"
LOG_FILE="$(mktemp)"
trap 'rm -f "$LOG_FILE"' EXIT

echo "[OpenMac test] Building UEFI kernel..."

if ! zig build; then
    echo "[FAIL] Kernel build failed."
    exit 1
fi

echo "[OpenMac test] Booting through QEMU/OVMF..."
echo "[OpenMac test] Timeout: ${TIMEOUT_SECONDS}s"

set +e
timeout --signal=TERM --kill-after=2s "${TIMEOUT_SECONDS}s" zig build run-uefi >"$LOG_FILE" 2>&1
QEMU_STATUS=$?
set -e

cat "$LOG_FILE"

# The kernel intentionally enters an infinite HLT loop after boot.
# Therefore timeout(1) returning 124 is expected when the kernel boots correctly.
if [ "$QEMU_STATUS" -ne 124 ]; then
    echo
    echo "[FAIL] QEMU did not reach the expected kernel idle loop."
    echo "[FAIL] Exit status: $QEMU_STATUS"
    exit 1
fi

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
        echo "[FAIL] Missing kernel output: $message"
        exit 1
    fi
done

echo
echo "[PASS] OpenMac kernel boot test passed."
