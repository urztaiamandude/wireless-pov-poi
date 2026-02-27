# AGENTS.md

## Cursor Cloud specific instructions

### Project overview

Wireless LED POV Poi System — the Teensy 4.1 (POV engine) and ESP32-S3 (WiFi/BLE co-processor) work together as one device. Both firmwares live in this repo and are buildable with PlatformIO.

| Component | Path | Command | Port |
|---|---|---|---|
| **Teensy 4.1 firmware** | `teensy_firmware/` | `pio run -e teensy41` (from repo root) | N/A |
| **ESP32-S3 firmware** | `esp32_firmware/` | `cd esp32_firmware && pio run -e esp32s3` | N/A |
| **React Web UI** | `esp32_firmware/webui/` | `npm run dev` | 3000 |
| **Mock API Server** | `esp32_firmware/test_webui_server.js` | `node esp32_firmware/test_webui_server.js` | 8765 |
| **Python image tools + tests** | `examples/` | `cd examples && python3 -m pytest test_*.py -v` | N/A |

> **Firmware upload:** The firmware build commands above only compile. To also upload to connected hardware, append `-t upload` (e.g. `pio run -e teensy41 -t upload` or `cd esp32_firmware && pio run -e esp32s3 -t upload`). Firmware upload and runtime testing require physical hardware, but **compilation/build is fully supported in the cloud environment** using PlatformIO.

### Key commands

- **Build Teensy 4.1**: `pio run -e teensy41` (from repo root; uses root `platformio.ini`)
- **Build ESP32-S3**: `cd esp32_firmware && pio run -e esp32s3` (uses `esp32_firmware/platformio.ini`; note: `-e esp32` is not a valid env in this repo)
- **Type check (Web UI)**: `cd esp32_firmware/webui && npx tsc --noEmit`
- **Build (Web UI)**: `cd esp32_firmware/webui && npm run build`
- **Dev server (Web UI)**: `cd esp32_firmware/webui && npm run dev` (port 3000, host 0.0.0.0)
- **Python tests**: `cd examples && python3 -m pytest test_*.py -v` (28 tests, requires Pillow)
- **Mock API**: `node esp32_firmware/test_webui_server.js` (port 8765, simulates ESP32-S3 endpoints)

### Non-obvious caveats

- **Two separate `platformio.ini` files**: The root `platformio.ini` has `src_dir = teensy_firmware`, so its environments (including the ESP32-S3 one) actually build the Teensy sources from `teensy_firmware/` rather than the ESP32 firmware in `esp32_firmware/`. For ESP32-S3 builds, always run `pio` from inside `esp32_firmware/`, which has its own `platformio.ini` configured for the ESP32 firmware. Note: the header comments at the top of the root `platformio.ini` that say "Build ESP32: pio run -e esp32s3" / "Upload ESP32: …" are outdated and refer to this root config; they actually build the Teensy sources despite the ESP32 label, so prefer the commands listed above.
- The Vite proxy in `esp32_firmware/webui/vite.config.ts` points to a hardcoded IP (`10.100.9.230`) which is the real ESP32-S3 device. Without hardware, API calls from the React UI will fail but the UI itself renders and is interactive. The mock API server at port 8765 serves the embedded ESP32-S3 web preview (a separate, standalone HTML interface), not the React UI.
- Use `python3` (not `python`) — the system does not have a `python` symlink.
- `pio` is installed to `~/.local/bin` — this is added to PATH via `~/.bashrc`.
- There is no ESLint configuration in the project; `tsc --noEmit` is the primary static analysis tool for the web UI.
- No pre-commit hooks, lint-staged, or Husky configured in this repo.
- See `CLAUDE.md` and `.github/copilot-instructions.md` for comprehensive project context, hardware constraints, and code style conventions.
