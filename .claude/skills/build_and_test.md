# Build and Test

How to build, test, and validate each component of the Wireless POV Poi system.

## Teensy 4.1 Firmware

```bash
# PlatformIO — compile only (upload requires hardware)
pio run -e teensy41            # from repo root; uses root platformio.ini

# Arduino IDE (recommended for production)
# 1. Open teensy_firmware/teensy_firmware.ino
# 2. Board: Teensy 4.1 | USB Type: Serial
# 3. Click Upload
```

## ESP32-S3 Firmware

```bash
# MUST run from esp32_firmware/ — it has its own platformio.ini
cd esp32_firmware && pio run -e esp32s3
```

> **Caveat**: The root `platformio.ini` also has an `esp32s3` env, but it builds the *Teensy* sources (`src_dir = teensy_firmware`). Always `cd esp32_firmware` first.

## Web UI (React + TypeScript)

```bash
cd esp32_firmware/webui
npm install                    # first time
npm run dev                    # dev server on localhost:3000
npm run build                  # production build to dist/
npx tsc --noEmit               # type check (no ESLint configured)
```

## Python Image Tools & Tests

```bash
cd examples
pip install Pillow pytest      # dependencies
python3 -m pytest test_*.py -v # 28 tests
```

> Use `python3` — there is no `python` symlink in this environment.

## Mock API Server

```bash
node esp32_firmware/test_webui_server.js   # port 8765, simulates ESP32 endpoints
```

This serves the standalone `web_preview.html`, not the React UI.
