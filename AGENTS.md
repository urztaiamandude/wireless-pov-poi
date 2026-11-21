# AGENTS.md

## Cursor Cloud specific instructions

### Project overview

Wireless LED POV Poi (Nebula Poi). The Teensy 4.1 drives the APA102 strip. The ESP32-S3 hosts WiFi, BLE, and both control UIs. Facts about hardware and what is actually implemented are in [README.md](README.md).

| Component | Path | Command | Port |
|---|---|---|---|
| **Teensy 4.1 firmware** | `firmware/teensy_firmware/` | `pio run -e teensy41` (from repo root) | N/A |
| **ESP32-S3 firmware** | `firmware/esp32_firmware/` | `cd firmware/esp32_firmware && pio run -e esp32s3` | N/A |
| **React Web UI** | `firmware/esp32_firmware/webui/` | `npm run dev` | 3000 |
| **Embedded simple UI** | PROGMEM `rootPage` in `firmware/esp32_firmware/esp32_firmware.ino` | served when SPIFFS has no `index.html` | N/A |
| **Mock API Server** | `firmware/esp32_firmware/test_webui_server.js` | `node firmware/esp32_firmware/test_webui_server.js` | 8765 |
| **Python image tools + tests** | `examples/` | `cd examples && python3 -m pytest test_*.py -v` | N/A |

> **Firmware upload:** The build commands above only compile. To upload, append `-t upload`. Upload needs hardware. Compilation in this environment uses PlatformIO.

PSRAM (2× 8 MB) is required on the Teensy; the sketch halts without it. Default sacrificial LED count is 0 (two SN74AHCT1G125 buffers). `COLOR_ORDER` is BGR.

### ⚠️ Hardware responsibility separation

The **Teensy 4.1 is the ONLY device physically connected to the APA102 LEDs**. All LED display processing (patterns, brightness, images, frame rendering) belongs in the Teensy firmware only. The **ESP32/ESP32-S3** hosts the web UI and acts as a WiFi/BLE bridge — it forwards settings to the Teensy via serial and should **NOT** enforce firmware-level LED display restrictions (brightness clamping, pattern bounds, LED index validation, etc.) because it does not control the LEDs.

### Key commands

- **Build Teensy 4.1**: `pio run -e teensy41` (from repo root; uses root `platformio.ini`)
- **Build ESP32-S3**: `cd firmware/esp32_firmware && pio run -e esp32s3` (`-e esp32` is not an environment)
- **Type check (Web UI)**: `cd firmware/esp32_firmware/webui && npx tsc --noEmit`
- **Build (Web UI)**: `cd firmware/esp32_firmware/webui && npm run build`
- **Dev server (Web UI)**: `cd firmware/esp32_firmware/webui && npm run dev` (port 3000, host 0.0.0.0)
- **Python tests**: `cd examples && python3 -m pytest test_*.py -v` (requires Pillow)
- **Mock API**: `node firmware/esp32_firmware/test_webui_server.js` (port 8765)

### Non-obvious caveats

- **Two `platformio.ini` files**: Root `src_dir` is `firmware/teensy_firmware` and that project builds the Teensy only. ESP32-S3 builds run inside `firmware/esp32_firmware`. That file pins `platform = espressif32@6.9.0` and `memory_type = qio_opi`. Do not bump the pin without a successful compile. Platform 8.x does not compile this sketch.
- The Vite dev server proxies `/api` to `VITE_API_PROXY_TARGET`, default `http://192.168.4.1`. Without hardware, those API calls fail; the UI still renders. The mock server on port 8765 is the standalone preview, not the React app.
- Use `python3` (not `python`) — the system does not have a `python` symlink.
- `pio` is installed to `~/.local/bin` — this is added to PATH via `~/.bashrc`.
- There is no ESLint configuration in the project; `tsc --noEmit` is the primary static analysis tool for the web UI.
- No pre-commit hooks, lint-staged, or Husky configured in this repo.
- See `CLAUDE.md` and `.github/copilot-instructions.md` for comprehensive project context, hardware constraints, and code style conventions.

### ⚠️ Non-compiled files — DO NOT apply firmware fixes here

These files are **NOT compiled into any firmware build** and changes to them have **NO effect** on the deployed device:

| File | What it is | Why it's excluded |
|------|-----------|-------------------|
| `firmware/esp32_firmware/web_preview.html` | Standalone browser preview | Not uploaded to SPIFFS, not served by the ESP32 |
| `firmware/esp32_firmware/test_webui_server.js` | Mock API server | Node.js dev tool only |

**Shipped UI code:**
1. `firmware/esp32_firmware/webui/` — React app, built to `dist/`, uploaded with `pio run -e esp32s3 -t uploadfs` from `firmware/esp32_firmware`
2. `firmware/esp32_firmware/esp32_firmware.ino` (`rootPage`) — embedded page when SPIFFS has no `index.html`

Keep both. Web UI bug fixes go in `webui/` and/or `rootPage`, not in `web_preview.html`.

`firmware/teensy_firmware/BatteryMonitor.h`, `BMPImageReader.h`, and `BMPImageSequence.h` are not included by the sketch. Do not treat them as running firmware.
