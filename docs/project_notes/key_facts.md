# POV Poi — Key Facts & Configuration Reference

Quick lookup for ports, pins, constants, and non-secret configuration.

---

## Hardware Identity

| Item | Value |
|------|-------|
| Project name | Nebula POV Poi (`wireless-pov-poi`) |
| Main controller | Teensy 4.1 @ 600 MHz |
| Co-processor | ESP32 / ESP32-S3 |
| LED strip | APA102, 32 physical LEDs |
| Default display LEDs | 31 (LED 0 sacrificial by default) |

---

## Pin Assignments

### Teensy 4.1

| Pin | Function |
|-----|----------|
| 11 | APA102 DATA (DI) |
| 13 | APA102 CLOCK (CI) |
| 0 (RX1) | UART RX ← ESP32 TX |
| 1 (TX1) | UART TX → ESP32 RX |

### ESP32 / ESP32-S3

| Pin | Function |
|-----|----------|
| GPIO 16 | UART RX ← Teensy TX1 |
| GPIO 17 | UART TX → Teensy RX1 |

---

## Runtime Defaults (Current Firmware)

| Setting | Value |
|---------|-------|
| AP SSID | `POV-POI-WiFi` |
| AP IP | `192.168.4.1` |
| Baud rate (Teensy ↔ ESP32) | 115200 |
| Brightness range | 0–255 |
| Frame-rate range | 10–1000 FPS |

---

## Firmware Locations (Current Repo Layout)

| Component | Path |
|-----------|------|
| ESP32 firmware | `firmware/esp32_firmware/esp32_firmware.ino` |
| Teensy firmware | `firmware/teensy_firmware/teensy_firmware.ino` |
| ESP32 BLE bridge | `firmware/esp32_firmware/src/ble_bridge.h` and `firmware/esp32_firmware/src/ble_bridge.cpp` |

---

## Key Source Directories

| Directory | Purpose |
|-----------|---------|
| `firmware/esp32_firmware/` | ESP32/ESP32-S3 firmware + web assets |
| `firmware/teensy_firmware/` | Teensy 4.1 firmware |
| `docs/` | Project documentation |
| `docs/project_notes/` | Project memory / tracking notes |
| `examples/` | Python image tools and tests |
| `scripts/` | Utility scripts |

---

## Image Tools

| Tool | Command |
|------|---------|
| GUI converter | `cd examples && python3 image_converter_gui.py` |
| CLI converter | `cd examples && python3 image_converter.py <image>` |

---

## Notes

- Teensy is the only controller physically driving APA102 LEDs.
- ESP32/ESP32-S3 hosts WiFi/BLE/web UI and forwards control data over UART.
