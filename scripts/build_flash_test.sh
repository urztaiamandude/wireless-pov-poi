#!/usr/bin/env bash
# Build, flash, and run all hardware tests.
# Usage:
#   ./scripts/build_flash_test.sh                     # build + flash + test
#   ./scripts/build_flash_test.sh --test-only          # skip build/flash
#   ./scripts/build_flash_test.sh --suite teensy       # teensy serial tests only
#   ./scripts/build_flash_test.sh --list-ports         # show serial ports
#
# Environment variables (override auto-detection):
#   TEENSY_PORT=/dev/ttyACM0
#   ESP32_PORT=/dev/ttyUSB0
#   ESP32_URL=http://192.168.4.1

set -euo pipefail
cd "$(dirname "$0")/.."

ARGS=()
TEST_ONLY=false

for arg in "$@"; do
    case "$arg" in
        --test-only)
            TEST_ONLY=true
            ;;
        *)
            ARGS+=("$arg")
            ;;
    esac
done

if [ "$TEST_ONLY" = false ]; then
    ARGS+=("--build" "--flash")
fi

# Pass port overrides if set
[ -n "${TEENSY_PORT:-}" ] && ARGS+=("--teensy-port" "$TEENSY_PORT")
[ -n "${ESP32_PORT:-}" ]  && ARGS+=("--esp32-port"  "$ESP32_PORT")
[ -n "${ESP32_URL:-}" ]   && ARGS+=("--esp32-url"   "$ESP32_URL")

# Save JSON report
ARGS+=("--report" "test_results.json")

python3 -m scripts.test_hardware.run_tests "${ARGS[@]}"
