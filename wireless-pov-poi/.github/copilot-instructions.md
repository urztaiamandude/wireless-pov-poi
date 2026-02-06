# Copilot Instructions for Nebula POV Poi

## Architecture Overview

Dual-microcontroller wireless POV LED poi system:
```
User → Web/App → ESP32 (WiFi AP @ 192.168.4.1) → Serial (115200 baud) → Teensy 4.1 → FastLED → APA102 LEDs
```

**⚠️ Critical LED Rule**: LED 0 is level-shifter only; LEDs 1-31 are display pixels. Always iterate from index 1:
```cpp
for (int i = 1; i < NUM_LEDS; i++) { leds[i] = color; }  // NEVER start from 0!
```

## Project Structure

| Directory | Purpose | Status |
|-----------|---------|--------|
| `teensy_firmware/` | Single-file Arduino IDE firmware | **Production-ready** - use for deployment |
| `firmware/teensy41/` | PlatformIO modular firmware | ⚠️ Incomplete - see note below |
| `esp32_firmware/` | WiFi AP + web server + REST API | Production-ready |
| `examples/` | Python image tools, GUI, tests | Complete with pytest suite |
| `docs/` | `API.md`, `WIRING.md` | Reference documentation |

**PlatformIO firmware gaps** (`firmware/teensy41/`): Missing full command handler parity with `teensy_firmware.ino`. The `esp32_interface.cpp` processes simple/structured protocols but sequence playback (mode 3) and some SD commands aren't fully wired. Use `teensy_firmware/` for production.

## Build Commands

```bash
# Build both firmwares (root platformio.ini)
pio run -e teensy41 -e esp32

# Python tests
cd examples && pip install Pillow && pytest test_*.py -v

```

## REST API Quick Reference

Base URL: `http://192.168.4.1` (or `http://povpoi.local` via mDNS)

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/status` | GET | Returns `{mode, index, brightness, framerate, connected}` |
| `/api/mode` | POST | Set mode & index: `{"mode": 2, "index": 0}` |
| `/api/brightness` | POST | `{"brightness": 128}` (0-255) |
| `/api/framerate` | POST | `{"framerate": 60}` (10-120) |
| `/api/image` | POST | Multipart upload, auto-converts to 31px wide |
| `/api/pattern` | POST | `{"type": 0, "color1": "#FF0000", "speed": 50}` |
| `/api/live` | POST | Raw RGB frame for live mode |

## Serial Protocol (ESP32 ↔ Teensy)

Simple protocol: `[0xFF][CMD][LEN][DATA...][0xFE]`
Structured protocol: `[TYPE:1][LEN_H:1][LEN_L:1][DATA:LEN][CHECKSUM:1]`

Command codes in `teensy_firmware.ino`: 0x01=Mode, 0x02=Image, 0x03=Pattern, 0x05=LiveFrame, 0x06=Brightness, 0x07=FrameRate, 0x10=Status

## Adding New Patterns

Patterns defined in `displayPattern()` switch. Types 0-15 exist (Rainbow→Music Sparkle). To add:
```cpp
// In teensy_firmware.ino displayPattern()
case 16:  // New pattern
  for (int i = 1; i < NUM_LEDS; i++) {  // Start from 1!
    leds[i] = CHSV(hue + i * 8, 255, 255);
  }
  break;
```
Update `MAX_PATTERNS` constant and ESP32 web interface pattern buttons.

## Music-Reactive Patterns (Types 11-15)

Requires MAX9814 microphone module on **Teensy pin A0**:
```
MAX9814 VDD  → 3.3V
MAX9814 GND  → GND
MAX9814 OUT  → Teensy A0
MAX9814 GAIN → Leave floating (60dB) or GND (50dB)
```
Audio config in `teensy_firmware.ino`: `AUDIO_PIN A0`, `AUDIO_SAMPLES 64`, `AUDIO_NOISE_FLOOR 50`

## Image Conversion Flow

All converters (Python/Web) must flip vertically for correct POV display:
```python
img = img.transpose(Image.FLIP_TOP_BOTTOM)  # Required!
```
Target size: 31 pixels wide (matches DISPLAY_LEDS), max 64 pixels tall.

## Testing

Python tests in `examples/` use pytest with Pillow-generated test images:
```bash
cd examples && pytest test_*.py -v
```

**Key test files:**
- `test_image_converter.py` - Conversion, resizing, aspect ratio
- `test_vertical_flip.py` - POV orientation validation
- `test_error_handling.py` - Invalid inputs, missing files

**Common test failures:**
- Missing Pillow: `pip install Pillow`
- Wrong working directory: must run from `examples/`
- Image dimension assertions: check 31px width, ≤64px height

## Key Constraints

- **Display modes**: 0=Idle, 1=Image, 2=Pattern, 3=Sequence, 4=Live
- **Ranges**: brightness 0-255, FPS 10-120, image max 31×64px
- **WiFi**: SSID `POV-POI-WiFi`, password `povpoi123`, IP `192.168.4.1`
- **Performance**: Teensy loop is time-critical - no blocking calls
- **Power**: Full brightness LEDs draw 2-3A

## SD Card (Optional)

Enable: Uncomment `#define SD_SUPPORT` in `teensy_firmware.ino`
- Uses Teensy 4.1 built-in SD slot (FAT32)
- Directories: `/poi_images/`, `/poi_patterns/`
- Commands: 0x20-0x23 (save/load/list/delete)

## Style Conventions

- **C++**: `camelCase` functions, `UPPER_CASE` constants, LED loops from index 1
- **Python**: PEP 8, type hints, Pillow for images
- **Web**: Vanilla ES6+ JS, mobile-first responsive

