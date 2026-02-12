# CLAUDE.md — Nebula Poi (Wireless POV Poi)

## Project Overview

Wireless persistence-of-vision (POV) LED poi system. Dual-processor design: **Teensy 4.1** (LED rendering @ 600MHz) + **ESP32/ESP32-S3** (WiFi/BLE networking @ 240MHz). Control via web UI, BLE, or Flutter app.

## Repository Structure

```
teensy_firmware/          # Arduino IDE Teensy firmware (single-file, recommended)
firmware/teensy41/        # PlatformIO Teensy firmware (modular, advanced)
esp32_firmware/           # ESP32/ESP32-S3 firmware + web interface
esp32_project/            # ESP32 PlatformIO project
esp32s3_project/          # ESP32-S3 PlatformIO project
wireless_pov_poi_app/     # Flutter mobile/web/desktop app (Dart)
examples/                 # Python image conversion tools + tests
scripts/                  # Build scripts + AI agent tools
docs/                     # API, wiring, BLE protocol, etc.
```

## Build Commands

### Firmware (PlatformIO)
```bash
pio run                              # Build all environments
pio run -e teensy41                  # Build Teensy firmware
pio run -e esp32                     # Build ESP32 firmware
pio run -e esp32s3                   # Build ESP32-S3 firmware
pio run -e teensy41 --target upload  # Upload Teensy firmware
pio run -e esp32s3 --target upload   # Upload ESP32-S3 firmware
pio check                            # Static analysis (cppcheck/clangtidy)
```

### Python Tools
```bash
cd examples
pip install -r requirements.txt      # Install deps (Pillow>=9.0.0)
pytest test_*.py -v                  # Run all tests
python image_converter.py <image>    # CLI image converter
python image_converter_gui.py        # GUI image converter
```

### Flutter App
```bash
cd wireless_pov_poi_app
flutter pub get                      # Install dependencies
flutter run                          # Run app
flutter build windows --release      # Build Windows
flutter build web --release          # Build Web
```

## Architecture

- **Serial protocol** between Teensy ↔ ESP32: binary framing `[0xFF][CMD][LEN][DATA...][0xFE]` at 115200 baud
- **Display modes**: 0=Idle, 1=Image, 2=Pattern, 3=Sequence, 4=Live
- **31 display LEDs** (APA102 RGB), LED 0 reserved for level shifting
- **WiFi AP**: SSID `POV-POI-WiFi`, password `povpoi123`, web UI at `http://192.168.4.1`
- **BLE**: Nordic UART Service for direct device control
- **State machine** pattern for mode management; non-blocking loop design

## Coding Conventions

### C++ (Firmware)
- Constants: `UPPER_SNAKE_CASE` (macros) or `kCamelCase` (compile-time)
- Functions/variables: `camelCase`
- Structs/Classes: `PascalCase`
- Fixed-size buffers only — no dynamic allocation in main loop
- Use `millis()` timing, never `delay()` in loop
- FastLED library for LED control

### Python
- PEP 8, snake_case functions/variables, PascalCase classes
- pytest for testing with descriptive test names

### Dart/Flutter
- Standard Dart conventions, Provider for state management
- Private members prefixed with `_`

### Git Commits
- Format: `Type: Brief description` (Add, Fix, Update, Docs, Refactor, Test, Chore)
- Wrap body at 72 chars, reference issues with `Fixes #N`

## Key Constants
```
NUM_LEDS = 32 (31 display + 1 level-shift)
MAX_IMAGES = 10
MAX_PATTERNS = 18
MAX_SEQUENCES = 5
SERIAL_BAUD = 115200
```

## Testing
- Python: `pytest` in `examples/`
- C++: `pio check` for static analysis
- Hardware: integration test checklist in docs
- All tests must pass before PR merge
