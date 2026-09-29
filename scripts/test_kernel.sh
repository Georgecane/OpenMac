#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

TIMEOUT_SECONDS="${OPENMAC_TEST_TIMEOUT:-8}"
OVMF_PATH="${OPENMAC_OVMF:-/usr/share/edk2/x64/OVMF.4m.fd}"
SERIAL_LOG="$(mktemp)"
DEBUG_LOG="$ROOT_DIR/zig-out/openmac-debug.log"
QEMU_LOG="$(mktemp)"
trap 'rm -f "$SERIAL_LOG" "$DEBUG_LOG" "$QEMU_LOG"' EXIT

echo "[OpenMac test] Building UEFI kernel..."

if ! zig build; then
    echo "[FAIL] Kernel build failed."
    exit 1
fi

if [ ! -f "$OVMF_PATH" ]; then
    echo
    echo "[FAIL] OVMF firmware not found:"
    echo "[FAIL] $OVMF_PATH"
    echo "[FAIL] Set OPENMAC_OVMF to the correct OVMF firmware path."
    exit 1
fi

rm -f "$DEBUG_LOG"

echo "[OpenMac test] Booting through QEMU/OVMF from the IDE FAT disk..."
echo "[OpenMac test] Timeout: ${TIMEOUT_SECONDS}s"
echo "[OpenMac test] OVMF: $OVMF_PATH"

set +e
timeout --signal=TERM --kill-after=2s "${TIMEOUT_SECONDS}s" \
    qemu-system-x86_64 \
    -bios "$OVMF_PATH" \
    -drive "file=fat:rw:$ROOT_DIR/zig-out/uefi,format=raw,if=ide" \
    -boot order=c,menu=off \
    -m 512M \
    -serial "file:$SERIAL_LOG" \
    -debugcon "file:$DEBUG_LOG" \
    -global isa-debugcon.iobase=0xe9 \
    -display none \
    -no-reboot \
    -no-shutdown \
    -d guest_errors,cpu_reset \
    -D "$QEMU_LOG"
QEMU_STATUS=$?
set -e

if [ -s "$SERIAL_LOG" ]; then
    echo
    echo "[OpenMac test] Serial output:"
    cat "$SERIAL_LOG"
fi

if [ -s "$QEMU_LOG" ]; then
    echo
    echo "[OpenMac test] QEMU diagnostics:"
    cat "$QEMU_LOG"
fi

if [ "$QEMU_STATUS" -ne 124 ]; then
    echo
    echo "[FAIL] QEMU did not remain running until the test timeout."
    echo "[FAIL] Exit status: $QEMU_STATUS"
    exit 1
fi

if [ ! -f "$DEBUG_LOG" ]; then
    echo
    echo "[FAIL] QEMU produced no kernel debug log."
    echo "[FAIL] QEMU diagnostics are shown above."
    exit 1
fi

echo
echo "[OpenMac test] Kernel debug trace:"
cat "$DEBUG_LOG"

required_debug_messages=(
    "UEFI:EfiMain"
    "UEFI:entered"
    "UEFI:boot-services"
    "UEFI:memory-map-info"
    "UEFI:map-buffer"
    "UEFI:memory-map"
    "UEFI:exit-boot-services"
    "UEFI:boot-services-exited"
    "UEFI:kernel-main"
    "KERNEL:entered"
    "KERNEL:serial-init"
    "KERNEL:trigger-ud"
    "KERNEL:exception-ud"
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
    "Triggering undefined instruction exception (#UD)."
    "Exception: undefined instruction (#UD)."
)

for message in "${required_messages[@]}"; do
    if ! grep -Fq "$message" "$SERIAL_LOG"; then
        echo
        echo "[FAIL] Missing serial output: $message"
        exit 1
    fi
done

echo
echo "[PASS] OpenMac undefined instruction exception test passed."
