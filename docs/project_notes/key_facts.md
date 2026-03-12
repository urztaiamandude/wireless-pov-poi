# POV Poi — Key Facts & Configuration Reference

Quick-lookup for ports, pins, constants, and non-secret configuration.  
⚠️  Never store passwords, API keys, or secrets here — use .env or a secrets manager.

---

## Hardware Identity

| Item | Value |
|------|-------|
| Project name | Nebula POV Poi (wireless-pov-poi) |
| Main controller | Teensy 4.1 @ 600 MHz |
| Co-processor (recommended) | ESP32-S3 N16R8 (16 MB Flash, 8 MB OPI PSRAM) |
| Co-processor (legacy supported) | ESP32 WROOM-32 (4 MB Flash, no PSRAM) |
| LED strip | APA102, 32 physical LEDs |
| Display LEDs | 31 (LED 1–31); LED 32 is level-shift sacrificial LED only |
| LED interface | SPI via FastLED |

---

## Pin Assignments — Teensy 4.1

| Pin | Function |
|-----|----------|
| 11 | APA102 DATA (DI) |
| 13 | APA102 CLOCK (CI) |
| 0 (RX1) | UART RX ← ESP32 TX |
| 1 (TX1) | UART TX → ESP32 RX |

---

## Pin Assignments — ESP32 / ESP32-S3

| Pin | Function |
|-----|----------|
| GPIO 16 (RX2) | UART RX ← Teensy TX1 |
| GPIO 17 (TX2) | UART TX → Teensy RX1 |

---

## Firmware Defines (Teensy — Arduino IDE build)

```cpp
#define NUM_LEDS      32    // physical LEDs on strip
#define DISPLAY_LEDS  31    // LEDs used for display (NUM_LEDS - 1)
#define DATA_PIN      11
#define CLOCK_PIN     13
#define SERIAL_BAUD   115200
```

---

## Image / Display Constraints

| Parameter | Value | Notes |
|-----------|-------|-------|
| Display height | **31 pixels** | Fixed — one per display LED |
| Display width | Variable | `round(src_width × (31 / src_height))` |
| Max brightness | 255 | Adjustable 0–255 |
| Frame rate range | 10–120 FPS | Adjustable |
| Recommended spin rate | 2–3 RPS | For good POV persistence |
| BMP format | 24-bit uncompressed | Required for BMPImageReader |

---

## WiFi Access Point

| Setting | Value |
|---------|-------|
| SSID | `POV-POI-WiFi` |
| Default IP | `192.168.4.1` |
| mDNS hostname | `http://povpoi.local` (unreliable on Windows — use IP) |
| Web UI port | 80 (HTTP) |

> ⚠️  The WiFi password is in the firmware source (`esp32_firmware/esp32_firmware.ino`). It is a default for a local-only AP — treat as low-security. Do not reuse it elsewhere.

---

## BLE Configuration

| Setting | Value |
|---------|-------|
| Device name | `Wireless POV Poi` |
| Profile | Nordic UART Service (NUS) |
| Service UUID | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| TX Characteristic | `6E400003-...` |
| RX Characteristic | `6E400002-...` |
| Compatible clients | nRF Connect, Flutter BLE, Web Bluetooth (Chrome/Edge) |

---

## Serial / UART (Inter-processor)

| Setting | Value |
|---------|-------|
| Baud rate | 115200 |
| Teensy port | Serial1 (TX1 pin 1, RX1 pin 0) |
| ESP32 port | Serial2 (GPIO 16/17) |

---

## Power Requirements

| Item | Spec |
|------|------|
| Supply voltage | 5 V |
| Minimum current | 2 A |
| Recommended current | 3 A |
| WiFi range (typical) | ~30 m |

---

## Firmware Locations

| Firmware | Path | Status |
|----------|------|--------|
| Teensy — Arduino IDE (**canonical**) | `teensy_firmware/teensy_firmware.ino` | ✅ Production-ready |
| Teensy — PlatformIO | `firmware/teensy41/` | ⚠️ Command processing incomplete |
| ESP32 / ESP32-S3 | `esp32_firmware/esp32_firmware.ino` | ✅ Production-ready |
| ESP32-S3 PlatformIO env | `esp32s3_project/` | ✅ |

---

## Key Source Directories

| Directory | Purpose |
|-----------|---------|
| `teensy_firmware/` | Arduino IDE Teensy firmware (canonical) |
| `firmware/teensy41/` | PlatformIO Teensy firmware (advanced) |
| `esp32_firmware/` | ESP32/ESP32-S3 Arduino IDE firmware |
| `esp32s3_project/` | ESP32-S3 PlatformIO project |
| `docs/` | All documentation |
| `docs/project_notes/` | Project memory (this system) |
| `examples/` | Image converter tools (GUI + CLI) |
| `scripts/` | Build scripts (Linux/Mac) |
| `plans/` | Design plans and specs |
| `wiki/` | Build guides |

---

## Image Converter Tools

| Tool | Command | Notes |
|------|---------|-------|
| GUI converter | `cd examples && python image_converter_gui.py` | Recommended |
| CLI converter | `cd examples && python image_converter.py <image>` | Batch-friendly |
| Web upload | `http://192.168.4.1` → Upload | Auto-converts on device |

---

## Demo Content (Pre-loaded in Firmware)

- 3 demo images: Smiley, Rainbow, Heart
- 5 demo patterns: Rainbow, Fire, Comet, Breathing, Plasma
- 1 demo sequence cycling through images and patterns

---

## Memory Allocation Rules

| Processor | Small buffers | Large buffers |
|-----------|--------------|---------------|
| Teensy 4.1 | Normal declaration | `EXTMEM` keyword required |
| ESP32-S3 | `malloc()` / `new` | `ps_malloc()` required |

Always verify PSRAM at boot: `Serial.printf("PSRAM: %u bytes\n", ESP.getPsramSize());`

---

## Multi-POI Sync

- Discovery: mDNS
- Topology: Peer-to-peer (no master/slave)
- Sync scope: Images, patterns, settings
- Standalone operation: Always available, sync is optional enhancement

---
