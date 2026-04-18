# Wireless POV Poi (Nebula Poi)

Wireless LED POV poi project with a dual-controller architecture:

- **Teensy 4.1** drives the APA102 LED strip and runs POV/pattern rendering.
- **ESP32/ESP32-S3** provides WiFi/BLE control, web APIs, and UI hosting.

> Status note: this repository is under active cleanup and has **not** been fully hardware-validated end-to-end in its current state.

## Current Repository Layout

- `firmware/teensy_firmware/` — Teensy firmware (`teensy_firmware.ino`)
- `firmware/esp32_firmware/` — ESP32 firmware (`esp32_firmware.ino` + BLE bridge sources)
- `firmware/esp32_firmware/webui/` — React + TypeScript web UI with Capacitor (Android/iOS folders present)
- `docs/` — project documentation
- `examples/` — Python conversion tools/tests

## Architecture (High Level)

User device (browser/app) → WiFi/BLE → ESP32/S3 → UART (115200) → Teensy 4.1 → SPI → APA102 LEDs

- Teensy UART pins: RX1=0, TX1=1
- ESP32 UART pins: RX=GPIO16, TX=GPIO17

## Build Commands

### Teensy firmware

```bash
cd /home/runner/work/wireless-pov-poi/wireless-pov-poi
pio run -e teensy41
```

### ESP32-S3 firmware

```bash
cd /home/runner/work/wireless-pov-poi/wireless-pov-poi/firmware/esp32_firmware
pio run -e esp32s3
```

### Web UI

```bash
cd /home/runner/work/wireless-pov-poi/wireless-pov-poi/firmware/esp32_firmware/webui
npm install
npx tsc --noEmit
npm run build
```

### Python examples/tests

```bash
cd /home/runner/work/wireless-pov-poi/wireless-pov-poi/examples
python3 -m pytest test_*.py -v
```

## Important Scope Notes

- Multi-poi synchronization code exists, but synchronized behavior should be treated as **experimental until validated on hardware**.
- No historical tree assumptions: the active firmware trees are only:
  - `firmware/teensy_firmware/`
  - `firmware/esp32_firmware/`

## License

MIT — see `LICENSE`.
