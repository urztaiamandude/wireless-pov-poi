# Post-Build/Flash Hardware Test Suite

Automated tests that verify the full Wireless POV Poi hardware stack after
building and flashing firmware.

## Prerequisites

```bash
pip install pyserial
```

## Quick Start

```bash
# Full pipeline: build firmware, flash both boards, run all tests
./scripts/build_flash_test.sh

# Windows
scripts\build_flash_test.bat
```

## Test Suites

### 1. Teensy Serial Tests (`--suite teensy`)
Direct USB serial communication with the Teensy. Tests every command in the
internal protocol (0xFF...0xFE):

- Connectivity (status request/response)
- Brightness control (min, 50%, max)
- Frame rate control (100fps, 30fps, 10fps)
- All display modes: Idle, 3 images, 15+ patterns, sequence
- Mode verification via status readback
- Pattern upload
- Live frame data
- Cleanup (return to idle)

### 2. ESP32 REST API Tests (`--suite api`)
HTTP endpoint tests against the ESP32 web server. Requires WiFi connection
to `POV-POI-WiFi` (password: `povpoi123`):

- WiFi reachability
- `GET /api/status` (schema validation)
- `POST /api/brightness` (set/restore)
- `POST /api/framerate`
- `POST /api/mode` (idle, image, pattern, sequence)
- `POST /api/pattern` (upload configuration)
- `POST /api/live` (31-pixel frame)
- `GET /api/sync/data` (image/pattern listings)
- `GET /api/sync/status`
- `GET /api/device/config`
- `GET /api/sd/list` and `/api/sd/info`

### 3. Integration Tests (`--suite integration`)
End-to-end verification: send commands via the ESP32 HTTP API, then read the
Teensy's serial status to confirm the command actually reached the hardware:

- Teensy serial link check
- ESP32 API link check
- Mode changes through API verified on Teensy serial
- Brightness through API verified via status readback
- Round-trip latency measurement (HTTP -> ESP32 -> Teensy -> ACK -> HTTP)

## Usage

```bash
# Run from project root
cd wireless-pov-poi

# List available serial ports
python3 -m scripts.test_hardware.run_tests --list-ports

# Run all suites (auto-detect ports)
python3 -m scripts.test_hardware.run_tests

# Teensy only
python3 -m scripts.test_hardware.run_tests --suite teensy --teensy-port /dev/ttyACM0

# ESP32 API only (connect to POV-POI-WiFi first)
python3 -m scripts.test_hardware.run_tests --suite api

# Custom ESP32 URL
python3 -m scripts.test_hardware.run_tests --suite api --esp32-url http://povpoi.local

# Build + flash + test
python3 -m scripts.test_hardware.run_tests --build --flash

# Save JSON report
python -m scripts.test_hardware.run_tests --report test_results.json
```

### Windows

```batch
REM Override port detection
set TEENSY_PORT=COM3
set ESP32_PORT=COM15
scripts\build_flash_test.bat

REM Test only (no build/flash)
scripts\build_flash_test.bat --test-only

REM Teensy tests only
scripts\build_flash_test.bat --test-only --suite teensy
```

### Linux/macOS

```bash
export TEENSY_PORT=/dev/ttyACM0
export ESP32_PORT=/dev/ttyUSB0
./scripts/build_flash_test.sh

# Test only
./scripts/build_flash_test.sh --test-only
```

## Output

The test runner prints a structured report:

```
========================================================================
  Teensy Serial Tests
========================================================================
  [+] Teensy connectivity  (45ms)
      mode=0 index=0 sd=False
  [+] Brightness min (0)  (32ms)
  [+] Brightness 50% (128)  (28ms)
  [+] Set mode: Pattern 0 (Rainbow)  (35ms)
  [+] Verify mode: Pattern 0 (Rainbow)  (42ms)
      Teensy confirmed mode=2 index=0
  [X] Set mode: Pattern 11 (VU Meter)  (501ms)
      No response (timeout)
------------------------------------------------------------------------
  Teensy Serial Tests: 5/6 passed, 1 failed, 0 skipped, 0 warnings  (2.1s)
========================================================================
```

Verdicts:
- `[+]` PASS - test succeeded
- `[X]` FAIL - test failed
- `[-]` SKIP - test skipped (e.g., no hardware)
- `[!]` WARN - test passed with warnings

JSON reports are saved when `--report <path>` is specified, suitable for CI
integration.

## Architecture

```
scripts/test_hardware/
    __init__.py             Package init
    protocol.py             Protocol encoding/decoding (0xFF/0xFE and 0xD0/0xD1)
    result.py               TestResult/TestReport data structures
    test_teensy_serial.py   Direct Teensy serial tests
    test_esp32_api.py       ESP32 REST API tests
    test_integration.py     End-to-end integration tests
    run_tests.py            CLI orchestrator (build, flash, test)
    README.md               This file
```
