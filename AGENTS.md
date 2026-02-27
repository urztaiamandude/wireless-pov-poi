# AGENTS.md

## Cursor Cloud specific instructions

### Project overview

Wireless LED POV Poi System with three locally-testable components:

| Component | Path | Build / Run command | Port |
|---|---|---|---|
| **React Web UI** | `esp32_firmware/webui/` | `npm run dev` | 3000 |
| **Mock API Server** | `esp32_firmware/test_webui_server.js` | `node esp32_firmware/test_webui_server.js` | 8765 |
| **Python image tools + tests** | `examples/` | `python3 -m pytest test_*.py -v` | N/A |
| **Teensy 4.1 firmware** ⚠️ hardware required | `teensy_firmware/` | Build: `pio run -e teensy41` · Upload: `pio run -e teensy41 -t upload` | N/A |
| **ESP32-S3 firmware** ⚠️ hardware required | `esp32_firmware/` | Build: `pio run -e esp32s3` · Upload: `pio run -e esp32s3 -t upload` | N/A |

> **Note:** The `pio run` commands above only compile the firmware. Adding `-t upload` flashes it to the connected device. Firmware testing requires physical hardware and cannot be done in the cloud environment.

### Key commands

- **Type check**: `cd esp32_firmware/webui && npx tsc --noEmit`
- **Build**: `cd esp32_firmware/webui && npm run build`
- **Dev server**: `cd esp32_firmware/webui && npm run dev` (port 3000, host 0.0.0.0)
- **Python tests**: `cd examples && python3 -m pytest test_*.py -v` (28 tests, requires Pillow)
- **Mock API**: `node esp32_firmware/test_webui_server.js` (port 8765, simulates ESP32 endpoints)

### Non-obvious caveats

- The Vite proxy in `esp32_firmware/webui/vite.config.ts` points to a hardcoded IP (`10.100.9.230`) which is the real ESP32 device. Without hardware, API calls from the React UI will fail but the UI itself renders and is interactive. The mock API server at port 8765 serves the embedded ESP32 web preview (a separate, standalone HTML interface), not the React UI.
- Use `python3` (not `python`) — the system does not have a `python` symlink.
- There is no ESLint configuration in the project; `tsc --noEmit` is the primary static analysis tool for the web UI.
- No pre-commit hooks, lint-staged, or Husky configured in this repo.
- See `CLAUDE.md` and `.github/copilot-instructions.md` for comprehensive project context, hardware constraints, and code style conventions.
