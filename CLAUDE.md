# CLAUDE.md - Project Context for AI Assistants

> **Quick Reference Guide** for Claude (or any AI assistant) working on the Wireless POV Poi project.
> Hardware, build commands, and what is actually implemented: [README.md](README.md). If this file disagrees with the README or the firmware, the README and the firmware win.

---

## Project Overview

**Wireless LED POV Poi System** - A persistence of vision LED display using rotating poi, featuring wireless control and real-time pattern generation.

### Core Hardware
- **Main Controller**: Teensy 4.1 @ 600MHz (ARM Cortex-M7)
  - 1MB Internal RAM
  - **Required**: 16MB PSRAM (2× 8MB chips). The firmware halts without it.
  - With PSRAM: up to 200 images at 32×400
- **WiFi Co-processor**: ESP32-S3 DevKitC-1 N16R8 (16MB Flash, 8MB PSRAM)
- **Display**: APA102 RGB LED strip (32 LEDs total)
  - Level shift: 2× SN74AHCT1G125 (data and clock). All 32 LEDs display.
  - Sacrificial-LED count defaults to 0 and stays runtime-configurable
  - LED `g_displayLedStart` (bottom) to LED `g_displayLedStart + g_displayLeds - 1` (top)
  - `COLOR_ORDER` is BGR. Verify with a known red on first power-up.
- **Optional**: MAX9814 microphone for music-reactive patterns
- **Optional**: microSD card (Teensy 4.1 built-in slot) for image storage

### System Architecture
```
User (Phone/Browser) 
    ↓ WiFi (192.168.4.1)
ESP32/S3 (Web Server + WiFi AP)
    ↓ Serial UART (115200 baud, TX1/RX1)
Teensy 4.1 (POV Engine + FastLED)
    ↓ SPI (Pins 11/13)
APA102 LED Strip (32 LEDs)
```

### ⚠️ Hardware Responsibility Separation — IMPORTANT

The **Teensy 4.1 is the ONLY device physically connected to the APA102 LEDs**. All LED display processing (rendering patterns, applying brightness, displaying images, frame timing) must be handled exclusively by the Teensy 4.1 firmware.

The **ESP32/ESP32-S3 is a WiFi/BLE bridge and web UI host**. It forwards user settings (brightness, frame rate, mode, pattern selection, image uploads) to the Teensy via serial UART. The ESP32/ESP32-S3 should **NOT** enforce firmware-level LED display restrictions (e.g., brightness clamping, pattern count limits, LED index validation) because it does not physically control the LEDs. Any display-related validation or processing belongs in the Teensy firmware only.

**When making changes:**
- LED rendering logic, display restrictions, brightness application → **Teensy firmware only** (`firmware/teensy_firmware/`)
- Web UI, API endpoints, settings relay, image upload handling → **ESP32 firmware** (`firmware/esp32_firmware/`)
- The ESP32 passes values through to the Teensy — it is not responsible for LED hardware constraints
- Basic input sanitization (valid integers, rejecting malformed requests) is still appropriate on the ESP32

---

## CRITICAL HARDWARE CONSTRAINTS - DO NOT MODIFY

### LED Configuration
- **Total physical LEDs: 32** (`leds[0]` through `leds[31]`)
- **Default sacrificial count: 0.** Level shift is two SN74AHCT1G125 buffers, so all 32 LEDs display.
- A sacrificial LED plus a MOSFET shifter is a fallback only. The count stays configurable.
- `NUM_LEDS` is always 32 (compile-time array size — do NOT change)
- The **display range is runtime-configurable** via the web UI → stored in EEPROM as `g_displayLeds` / `g_displayLedStart`; no recompile needed
- Saved EEPROM (magic `0xA5`) overrides the compile-time default until the UI writes a new value

### Teensy 4.1 <-> ESP32-S3 Serial Communication
- **Teensy TX:** Pin 1 / **Teensy RX:** Pin 0
- **ESP32-S3 TX:** Pin 17 / **ESP32-S3 RX:** Pin 16
- This UART serial link is the **only** communication path between the two processors
- All SD card operations (file listing, file read, file write) are routed through this link
- Do NOT attempt to access the SD card from the ESP32-S3 via SPI or any other protocol
- Do NOT change these pin assignments without updating both Teensy and ESP32-S3 firmware

### SD Card File System - Teensy 4.1
- SD card is mounted on the **Teensy 4.1**, not the ESP32-S3
- ESP32-S3 accesses SD card contents via serial communication with Teensy, not directly
- Web UI SD card explorer fetches file listings and data through the ESP32-S3 REST API, which in turn requests it from Teensy
- `/poi_images/*.pov` — POV image frames
- `/poi_patterns/*.pat` — pattern presets
- Filenames are up to 32 characters in the sketch (`MAX_FILENAME_LEN`). Avoid spaces.
- Do not document `/images/`, `/palettes/`, or `/config/`. Those directories are not what the sketch creates.
- Do NOT hardcode file paths in the UI
- All SD operations are asynchronous — UI must handle loading states and timeouts gracefully

## Web UI Constraints - Firmware Deployment

Two UIs share the ESP32 REST API. Do not remove either.

1. **Embedded page** (`rootPage` in `firmware/esp32_firmware/esp32_firmware.ino`), served when SPIFFS has no `/index.html`. This page is hand-written HTML. It must not load a CDN, must not use a build step, and must call relative API paths.
2. **React app** in `firmware/esp32_firmware/webui/`. It is built with npm/Vite and uploaded to SPIFFS. Source may use imports and a dev server. The uploaded files must still call relative API paths (`/api/status`, not an absolute LAN URL) and must not fetch third-party hosts at runtime.

### Absolutely Prohibited in the embedded page and in uploaded UI assets
- External CDN dependencies (no unpkg, cdnjs, jsdelivr, etc.)
- `fetch()` calls to external URLs
- Hard-coded device IPs in API calls

### Web UI Pre-Commit Checklist
- [ ] No external resource loads in browser network tab
- [ ] All API endpoints use relative paths
- [ ] Works with no internet connection
- [ ] Opens and renders correctly via `file://` with no development server

---

## Critical Design Constraints

### 1. LED Array Layout
```
Physical LED Strip (default configuration, sacrificial count 0):
┌────┬────┬────┬─────┬────┐
│  0 │ 1  │... │ 30  │ 31 │
└────┴────┴────┴─────┴────┘
↑ Display pixels 0 .. 31
g_displayLedStart=0, g_displayLeds=32
```

**ALL display code MUST:**
- Use `g_displayLedStart` and `g_displayLeds`: `for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++)`
- Default values: `g_displayLedStart = 0`, `g_displayLeds = 32`

### 2. Image Orientation & Dimensions

**POV Display Orientation:**
- **HEIGHT = `g_displayLeds` pixels** (default 32 — runtime-configurable via web UI)
- **WIDTH = variable** (calculated from aspect ratio, max 400px with PSRAM)
- LED strip forms the VERTICAL axis when spinning
- LED `g_displayLedStart` (bottom of strip) = bottom of image
- LED `g_displayLedStart + g_displayLeds - 1` (top of strip) = top of image
- Images scroll horizontally as poi spins

**Image Storage Format:**
```cpp
// Storage: pixels[x][y] where y is display LED index (0 = g_displayLedStart)
CRGB pixels[IMAGE_MAX_WIDTH][IMAGE_HEIGHT];  // IMAGE_HEIGHT = 32 (compile-time array max)
CRGB pixels[IMAGE_WIDTH][IMAGE_HEIGHT];  // Max 32x400

// Display mapping (NO flip needed):
leds[y] = pixels[current_column][y];  // y ranges 0-31
```

### 3. Communication Protocol

**Serial Communication (Teensy ↔ ESP32):**
- Baud rate: 115200
- Teensy TX1 (Pin 1) → ESP32 RX (GPIO 16)
- Teensy RX1 (Pin 0) ← ESP32 TX (GPIO 17)
- Binary frames for every command, including control: `0xFF … 0xFE`. Not text.

**WiFi Network:**
- SSID: `POV-POI-WiFi`
- Password: `povpoi123`
- IP: `192.168.4.1`
- mDNS hostname is `povpoi-<id>.local`, printed at boot. Not `povpoi.local`.

### 4. Power Requirements
- **Bring-up**: 5 V, 4 A bench supply
- **Portable**: 3S 1300 mAh XT60, no BMS, Pololu D24V50F5, INA219 in the handle, 10 A switch
- **Distribution**: Common 5 V rail for Teensy, ESP32-S3, and the LED strip
- **Ground**: Common ground for ALL components

---

## Firmware Architecture

### One Teensy sketch

`firmware/teensy_firmware/teensy_firmware.ino` is the only Teensy firmware. PlatformIO (root `platformio.ini`, env `teensy41`) is the preferred build. Arduino IDE opens the same file. There is no `firmware/teensy41/` tree.

`BatteryMonitor.h`, `BMPImageReader.h`, and `BMPImageSequence.h` sit next to the sketch and are **not** `#include`d.

### Current Feature Status

In the sketch and the ESP32 firmware:

- Image display, patterns 0–18, sequences, brightness, frame rate
- SD card (`SD_SUPPORT` is on; `/poi_images`, `/poi_patterns`)
- WiFi, BLE, REST API, live frames
- Multi-poi sync code (experimental on hardware)
- Two UIs: embedded page, and the React app when SPIFFS has `index.html`

Not in the build: battery monitoring, OTA, the BMP helper classes.

---

## Display Modes & Features

### Display Modes
1. **Idle** - Off/standby
2. **Image** - Display stored POV image
3. **Pattern** - Animated pattern (18 types)
4. **Sequence** - Playlist of images/patterns
5. **Live** - Real-time drawing from web interface

### Pattern Types (0-17)
**Basic Patterns (0-10):**
- 0: Rainbow, 1: Wave, 2: Gradient, 3: Sparkle, 4: Fire
- 5: Comet, 6: Breathing, 7: Strobe, 8: Meteor, 9: Wipe, 10: Plasma

**Music-Reactive (11-15)** (requires microphone):
- 11: VU Meter, 12: Pulse, 13: Audio Rainbow, 14: Center Burst, 15: Audio Sparkle

**Advanced (16-18):**
- 16: Split Spin, 17: Theater Chase, 18: Retro strobe

### Controls
- **Brightness**: 0-255 (adjustable via web/API)
- **Frame Rate**: 10-1000 FPS (adjustable; 500+ recommended for POV spinning)
- **Pattern Speed**: 1-255 (higher = faster)

---

## File Structure

```
wireless-pov-poi/
├── platformio.ini                 # Teensy build only
├── firmware/teensy_firmware/      # The Teensy sketch
│   ├── teensy_firmware.ino
│   ├── BatteryMonitor.h           # not included
│   ├── BMPImageReader.h           # not included
│   └── BMPImageSequence.h         # not included
├── firmware/esp32_firmware/       # ESP32-S3 firmware + both UIs
│   ├── platformio.ini
│   ├── esp32_firmware.ino         # embedded simple page
│   ├── webui/                     # React app, uploaded to SPIFFS
│   └── web_preview.html           # dev preview, not served by the ESP32
├── docs/                          # README.md at repo root is the source of truth
└── examples/                      # Python image tools and tests
```

### ⚠️ Non-Compiled Files — DO NOT apply firmware fixes here

The following files are **NOT compiled into any firmware build**. Do not apply bug fixes, input validation, or security patches to these files expecting them to ship on hardware:

| File | Purpose | Why it's excluded |
|------|---------|-------------------|
| `firmware/esp32_firmware/web_preview.html` | Standalone browser preview of the web UI | Not uploaded to SPIFFS, not served by ESP32 |
| `firmware/esp32_firmware/test_webui_server.js` | Mock API server for local development | Node.js dev tool only |

**The actual shipped web UI code lives in:**
1. **`firmware/esp32_firmware/webui/`** — React app built to `dist/`, uploaded with `pio run -e esp32s3 -t uploadfs` from `firmware/esp32_firmware`
2. **`firmware/esp32_firmware/esp32_firmware.ino`** (PROGMEM `rootPage`) — embedded page served when SPIFFS has no `index.html`

---

## Common Development Tasks

### Building Firmware

**PlatformIO (preferred):**
```bash
# Teensy, from the repository root
pio run -e teensy41

# ESP32-S3, from its own project. Never from the root platformio.ini.
# platformio.ini there pins espressif32@6.9.0 and memory_type qio_opi.
cd firmware/esp32_firmware && pio run -e esp32s3
```

**Arduino IDE:** open `firmware/teensy_firmware/teensy_firmware.ino` or `firmware/esp32_firmware/esp32_firmware.ino`. See `docs/FLASHING.md`.

### Image Conversion
```bash
# GUI converter (recommended)
cd examples
python image_converter_gui.py

# CLI converter
python image_converter.py input_image.jpg

# Web upload (automatic conversion)
# Upload at <http://192.168.4.1>
```

### Testing
```bash
# Serial debugging
python scripts/capture_serial_debug.py

# Standalone Teensy test
python test_teensy_standalone.py

# BLE protocol test
cd examples
python test_ble_protocol.py
```

---

## API Quick Reference

**Base URL:** `http://192.168.4.1/api/`

**Key Endpoints:**
- `GET /status` - System status
- `POST /brightness` - Set brightness (0-255)
- `POST /framerate` - Set frame rate (10-1000)
- `POST /mode` - Change display mode
- `POST /pattern` - Configure pattern
- `POST /image/upload` - Upload image (auto-converts to 32px height)
- `GET /image/list` - List stored images
- `POST /live/frame` - Send live frame data

See `docs/API.md` for complete reference.

---

## Code Style & Conventions

### Variable Naming
- `camelCase` for local variables
- `PascalCase` for structs/classes
- `UPPER_CASE` for constants/defines
- Descriptive names: `currentPatternIndex` not `idx`

### LED Index Usage
```cpp
// ✅ CORRECT - Use runtime display range variables
for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
  leds[i] = CRGB::Red;
}
```

### Image Pixel Access
```cpp
// ✅ CORRECT - Map display pixel index y (0-based) to LED index
for (int y = 0; y < g_displayLeds; y++) {
  leds[y + g_displayLedStart] = image.pixels[column][y];
}

// Display logic handles orientation naturally
```

### Pattern Speed Guidelines
- Slow: 20-40
- Medium: 50-80
- Fast: 100-150
- Very fast: 150+

---

## Memory Considerations

### Teensy 4.1
- **Internal RAM**: 1MB total
- **Flash**: 8MB total
- **PSRAM (required)**: 16MB (2× 8MB chips). The sketch halts if they are missing.
  - Up to 200 images, 32×400
  - See [PSRAM Installation Guide](docs/PSRAM_INSTALLATION.md)
- **Pattern storage**: Minimal (~100 bytes per pattern)
- **Performance**: PSRAM is 2-3x slower than internal RAM but sufficient for image storage

### ESP32-S3 N16R8
- **Flash**: 16MB (recommended for future expansion)
- **PSRAM**: 8MB (external RAM for web server buffering)
- **Web server**: Handles image upload/storage temporarily

---

## Troubleshooting Quick Reference

### No LED output
1. Check the 5 V 4 A supply and both SN74AHCT1G125 buffers (VCC 5 V, OE to GND)
2. Verify APA102 connections (Data=Pin 11, Clock=Pin 13, through the buffers)
3. Read the Teensy serial port. `FATAL` means PSRAM is missing.
4. If a known red looks blue, `COLOR_ORDER` is wrong (firmware default is BGR)

### WiFi connection issues
1. Verify ESP32 is powered and programmed
2. Check SSID/password in `esp32_firmware.ino`
3. Try direct IP: `192.168.4.1`
4. Check serial monitor for ESP32 boot messages

### Serial communication problems
1. Verify baud rate: 115200 on both devices
2. Check TX/RX connections (TX→RX, RX→TX)
3. Monitor Serial1 on Teensy for incoming commands
4. Check ESP32 serial output for command echo

### Image orientation incorrect
- Images should display correctly without manual flipping
- LED 0 = bottom of strip = bottom of image
- If upside down, check physical LED strip orientation
- See `docs/POV_DISPLAY_ORIENTATION_GUIDE.md`

---

## Safety & Best Practices

1. **Bring-up on a 5 V 4 A supply.** The portable pack has no BMS.
2. **Test patterns at low brightness first** (< 50)
3. **Common ground for all components** (critical!)
4. **Secure all connections** before spinning poi
5. **Never look directly at LEDs** at full brightness
6. **Use proper insulation** on all electrical connections

---

## External Resources

- **FastLED Library**: <https://github.com/FastLED/FastLED>
- **Teensy 4.1 Docs**: <https://www.pjrc.com/store/teensy41.html>
- **APA102 Datasheet**: <https://cdn-shop.adafruit.com/product-files/2343/APA102C.pdf>
- **ESP32-S3 Docs**: <https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/>

---

## Recent Changes & Active Development

**Last Major Update:** Peer-to-peer synchronization feature
- Multiple poi can discover and sync with each other
- Bidirectional sharing of images, patterns, settings
- See `docs/POI_PAIRING.md` for details

**Current Focus:**
- Hardware rebuild (level shifters, PSRAM, power plan in the README)
- Battery monitor is not integrated
- OTA firmware updates are not implemented

---

## Questions? Check These First

1. **"How do I change WiFi credentials?"** → `ssid` and `password` in `firmware/esp32_firmware/esp32_firmware.ino`
2. **"Images are upside down"** → `docs/POV_DISPLAY_ORIENTATION_GUIDE.md`
3. **"Can I use more LEDs?"** → Not on this strip. `NUM_LEDS` stays 32. Sacrificial count defaults to 0.
4. **"Battery power?"** → `docs/HARDWARE_BOM.md`. No BMS. The monitor header is not in the build.
5. **"Multiple poi sync?"** → `docs/POI_PAIRING.md`. Code exists; hardware sync is unproven.

---

**Related:** [README.md](README.md) is the setup guide. `docs/README.md` is the map of the other documents.
