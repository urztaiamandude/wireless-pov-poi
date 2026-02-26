# CLAUDE.md - Project Context for AI Assistants

> **Quick Reference Guide** for Claude (or any AI assistant) working on the Wireless POV Poi project.
> This file provides essential context to maintain consistency across all coding sessions.

---

## Project Overview

**Wireless LED POV Poi System** - A persistence of vision LED display using rotating poi, featuring wireless control and real-time pattern generation.

### Core Hardware
- **Main Controller**: Teensy 4.1 @ 600MHz (ARM Cortex-M7)
  - 1MB Internal RAM
  - **Optional**: 16MB PSRAM (2× 8MB chips) for expanded storage
  - With PSRAM: 50 images at 32×400px, without: 10 images at 32×200px
- **WiFi Co-processor**: ESP32-S3 N16R8 (16MB Flash, 8MB PSRAM) *recommended*, or standard ESP32
- **Display**: APA102 RGB LED strip (32 LEDs total)
  - All 32 LEDs used for display (hardware level shifter)
  - LED 0-31: Display pixels (32 pixels vertical)
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

---

## Critical Design Constraints

### 1. LED Array Layout
```
⚠️ CRITICAL: All 32 LEDs are used for display - hardware level shifter is used!

Physical LED Strip:
┌────┬────┬────┬────┬─────┬────┐
│ 0  │ 1  │ 2  │... │ 30  │ 31 │
└────┴────┴────┴────┴─────┴────┘
  ↑───────────────────────────↑
      Display pixels (32 total)
```

**ALL display code MUST:**
- Use `NUM_LEDS` (32) for loops: `for (int i = 0; i < NUM_LEDS; i++)`
- Use `DISPLAY_LEDS` (32) for height calculations
- Use `DISPLAY_LED_START` (0) as first display index

### 2. Image Orientation & Dimensions

**POV Display Orientation:**
- **HEIGHT = 32 pixels** (FIXED - one pixel per display LED)
- **WIDTH = variable** (calculated from aspect ratio, max 400px with PSRAM)
- LED strip forms the VERTICAL axis when spinning
- LED 0 (bottom of strip) = bottom of image
- LED 31 (top of strip) = top of image
- Images scroll horizontally as poi spins

**Image Storage Format:**
```cpp
// Storage: pixels[x][y] where y is LED index
CRGB pixels[IMAGE_WIDTH][IMAGE_HEIGHT];  // Max 32x400

// Display mapping (NO flip needed):
leds[y] = pixels[current_column][y];  // y ranges 0-31
```

### 3. Communication Protocol

**Serial Communication (Teensy ↔ ESP32):**
- Baud rate: 115200
- Teensy TX1 (Pin 1) → ESP32 RX2 (GPIO 16)
- Teensy RX1 (Pin 0) → ESP32 TX2 (GPIO 17)
- Binary protocol for image data, text commands for control

**WiFi Network:**
- SSID: `POV-POI-WiFi`
- Password: `povpoi123`
- IP: `192.168.4.1`
- Access: `http://povpoi.local` or `http://192.168.4.1`

### 4. Power Requirements
- **Voltage**: 5V DC
- **Current**: 2-3A (full brightness, all LEDs white)
- **Distribution**: Common 5V rail for Teensy, ESP32, and LED strip
- **Ground**: Common ground for ALL components

---

## Firmware Architecture

### Two Implementations Available

**1. Arduino IDE Firmware** (`teensy_firmware/`) - **RECOMMENDED**
- Single file: `teensy_firmware.ino` (1681 lines)
- Complete features: images, patterns, sequences, SD card support
- Quick setup, easier to modify
- Best for: Most users, rapid development

**2. PlatformIO Firmware** (`firmware/teensy41/`) - Advanced
- Modular architecture with separate components
- Professional build system
- Best for: Large-scale customization, team development
- ⚠️ Some features still being ported from Arduino version

### Current Feature Status

✅ **Fully Implemented:**
- Image display (upload, store, display)
- 16 animated patterns (basic + music-reactive)
- Sequences (chain images/patterns with durations)
- Brightness & frame rate control
- WiFi/BLE connectivity
- Web interface with REST API
- Peer-to-peer synchronization
- Live drawing mode

🚧 **PlatformIO Firmware Only (Partial):**
- Advanced SD card integration
- Some command processing features

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

**Advanced (16-17):**
- 16: Split Spin, 17: Theater Chase

### Controls
- **Brightness**: 0-255 (adjustable via web/API)
- **Frame Rate**: 10-120 FPS (adjustable)
- **Pattern Speed**: 1-255 (higher = faster)

---

## File Structure

```
wireless-pov-poi/
├── teensy_firmware/           # Teensy 4.1 firmware (Arduino IDE) ⭐
│   ├── teensy_firmware.ino   # Main firmware (1681 lines)
│   ├── BMPImageReader.h      # SD card BMP reading
│   └── BMPImageSequence.h    # Image playlist management
├── esp32_firmware/            # ESP32/S3 firmware
│   ├── esp32_firmware.ino    # WiFi + web server + BLE
│   ├── webui/                # React web UI (deployed to ESP32 filesystem)
│   └── web_preview.html      # Standalone UI preview (no hardware needed)
├── firmware/teensy41/         # PlatformIO version (advanced)
├── docs/                      # Complete documentation
│   ├── WIRING.md             # Hardware connections
│   ├── API.md                # REST API reference
│   ├── BLE_PROTOCOL.md       # Bluetooth Low Energy commands
│   ├── POI_PAIRING.md        # Multi-device sync guide
│   └── README.md             # Setup guide
├── examples/                  # Tools and utilities
│   ├── image_converter.py    # CLI image converter
│   └── image_converter_gui.py # GUI image converter
└── scripts/                   # Build and utility scripts
```

---

## Common Development Tasks

### Building Firmware

**Arduino IDE (Recommended):**
```bash
# 1. Open teensy_firmware/teensy_firmware.ino
# 2. Select Board: Teensy 4.1
# 3. Select USB Type: Serial
# 4. Click Upload
```

**PlatformIO:**
```bash
# Teensy
pio run -e teensy41 -t upload

# ESP32
pio run -e esp32 -t upload

# ESP32-S3 (recommended for new builds)
pio run -e esp32s3 -t upload
```

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
- `POST /framerate` - Set frame rate (10-120)
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
// ✅ CORRECT - All 32 LEDs are display LEDs
for (int i = 0; i < NUM_LEDS; i++) {
  leds[i] = CRGB::Red;
}
```

### Image Pixel Access
```cpp
// ✅ CORRECT - Direct mapping (no flip)
for (int y = 0; y < NUM_LEDS; y++) {
  leds[y] = image.pixels[column][y];
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
- **PSRAM (Optional)**: 16MB (2× 8MB chips soldered to board)
  - **With PSRAM**: ~1.8MB for 50 images (32×400 pixels each)
  - **Without PSRAM**: ~60KB for 10 images (32×200 pixels each)
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
1. Check power supply (5V, 2-3A)
2. Verify APA102 connections (Data=Pin 11, Clock=Pin 13)
3. Check LED 0 is connected (required for level shifting)
4. Test with simple pattern: `leds[1] = CRGB::Red; FastLED.show();`

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

1. **Always use current-limited power supply** (2-3A minimum)
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
- PlatformIO firmware feature parity
- Advanced SD card integration
- OTA firmware updates (planned)

---

## Questions? Check These First

1. **"How do I change WiFi credentials?"** → Edit `esp32_firmware.ino`, lines with `ssid` and `password`
2. **"Images are upside down"** → Check `docs/POV_DISPLAY_ORIENTATION_GUIDE.md`
3. **"Can I use more LEDs?"** → Yes, change `NUM_LEDS`, but keep LED 0 as level shift
4. **"Battery power?"** → Requires 5V battery with 2-3A capacity, add power management
5. **"Multiple poi sync?"** → See `docs/POI_PAIRING.md`

---

**Last Updated:** 2025-02-13
**Maintainer:** Project documentation automatically generated
**Related:** See `docs/README.md` for complete setup guide
