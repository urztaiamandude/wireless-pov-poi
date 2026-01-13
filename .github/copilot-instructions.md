# Copilot Instructions for Nebula POV Poi

## Architecture Overview

Wireless POV LED poi system with dual-microcontroller design:
```
User → Web Interface → ESP32 (WiFi AP @ 192.168.4.1) → Serial (115200 baud) → Teensy 4.1 → FastLED → APA102 LEDs
```

**Critical LED Detail**: LED 0 is for level shifting; display uses LEDs 1-31 (31 pixels). Always iterate from index 1:
```cpp
for (int i = 1; i < NUM_LEDS; i++) { leds[i] = color; }  // NOT from 0!
```

## Project Structure

| Directory | Purpose | When to Use |
|-----------|---------|-------------|
| `teensy_firmware/` | Arduino IDE firmware (single file) | **Most users** - simpler setup, complete features |
| `firmware/teensy41/` | PlatformIO modular firmware | Advanced users needing new features in development |
| `esp32_firmware/` | WiFi controller & web server | Always needed |
| `examples/` | Python image tools | Image conversion, GUI tools |
| `POVPoiApp/` | Android companion app | Mobile control |
| `docs/` | API & wiring reference | `API.md`, `WIRING.md` |

## Build Commands

```bash
# PlatformIO
pio run -e teensy41 && pio run -e esp32    # Build both firmwares
pio run -e teensy41 -t upload              # Upload to Teensy

# Python tools
cd examples && pip install -r requirements.txt
pytest test_*.py                            # Run tests

# Android app
cd POVPoiApp && ./gradlew assembleDebug    # Build APK
```

## SD Card Storage

SD card provides persistent storage for custom content (alternative to wireless transfer):
- **Location**: Teensy 4.1 built-in SD slot
- **Enable**: Uncomment `#define SD_SUPPORT` in `teensy_firmware.ino`
- **Directory**: `/poi_images/` for user content
- **Format**: FAT32, files named `image_XX.pov` or `pattern_XX.pov`

## Pattern System

Patterns are defined in `displayPattern()` using FastLED. Types: 0=Rainbow, 1=Wave, 2=Gradient, 3=Sparkle, 4=Fire, 5=Comet, 6=Breathing, 7=Strobe, 8=Meteor, 9=Wipe, 10=Plasma, 11=Music VU, 12=Music Pulse, 13=Music Rainbow, 14=Music Center, 15=Music Sparkle

```cpp
// Pattern structure
struct Pattern {
  uint8_t type;     // Pattern type ID
  CRGB color1;      // Primary color
  CRGB color2;      // Secondary color (gradients)
  uint8_t speed;    // Animation speed (1-255)
  bool active;
};

// Adding a new pattern - add case in displayPattern() switch
case 12:  // Your new pattern
  for (int i = 1; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, 255, 255);  // Your effect logic
  }
  break;
```

**Music Pattern**: Requires analog microphone on pin A0 (MAX9814 recommended).

**SD Pattern Presets**: With `SD_SUPPORT` enabled, patterns can be saved/loaded:
- Save: `savePatternPreset("mypreset")` → `/poi_patterns/mypreset.pat`
- Load: `loadPatternPreset("mypreset")`
- List: `listPatternPresets()`

## Serial Protocol (ESP32 ↔ Teensy)

Binary format: `[TYPE:1][LEN_H:1][LEN_L:1][DATA:LEN][CHECKSUM:1]`
- See [esp32_interface.cpp](firmware/teensy41/src/esp32_interface.cpp) for implementation
- Always validate checksums, handle partial reads with timeout

## Key Constraints

- **Display modes**: 0=Idle, 1=Image, 2=Pattern, 3=Sequence, 4=Live
- **Valid ranges**: brightness 0-255, FPS 10-120, image max 31×64px
- **WiFi**: SSID `POV-POI-WiFi`, password `povpoi123`, IP `192.168.4.1`
- **Teensy loop is time-critical**: No blocking calls, minimize `FastLED.show()` calls
- **Image flip**: Always `img.transpose(Image.FLIP_TOP_BOTTOM)` for correct POV orientation

## Style Conventions

- C++: `camelCase` functions, `UPPER_CASE` constants, iterate LEDs from index 1
- Python: PEP 8 with type hints, use Pillow for images
- Web: Vanilla JS (ES6+), mobile-first responsive

## Common Gotchas

- LED 0 is NOT for display (level shifter) - always start loops at index 1
- Serial is binary protocol, not human-readable text
- Images need vertical flip for correct POV orientation
- Full brightness draws 2-3A - size power supply accordingly
- SD card must be FAT32 formatted
