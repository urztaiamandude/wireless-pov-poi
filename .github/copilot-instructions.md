# Copilot Instructions for Nebula POV Poi

## Quick Reference

This repository contains a wireless LED POV (Persistence of Vision) poi system with dual microcontrollers:
- **Teensy 4.1**: High-performance LED controller and POV engine
- **ESP32/ESP32-S3**: WiFi/BLE co-processor for wireless control
- **Web UI**: React + TypeScript + Vite + Tailwind CSS interface

## Architecture Overview

Dual-microcontroller wireless POV LED poi system:
```
User (Phone/Browser) 
    ↓ WiFi (192.168.4.1) or BLE
ESP32/S3 (Web Server + WiFi AP)
    ↓ Serial UART (115200 baud, TX1/RX1)
Teensy 4.1 (POV Engine + FastLED)
    ↓ SPI (Pins 11/13)
APA102 LED Strip (32 LEDs)
```

## Critical Design Constraints

### ⚠️ LED Array Layout - ALWAYS FOLLOW THIS

**CRITICAL**: LED 0 is NEVER used for display - it's reserved for hardware level shifting!

```
Physical LED Strip:
┌────┬────┬────┬────┬─────┬────┐
│ 0  │ 1  │ 2  │... │ 30  │ 31 │
└────┴────┴────┴────┴─────┴────┘
  ↑    ↑─────────────────────↑
Level   Display pixels (31 total)
Shift
```

**ALL display code MUST:**
- Start loops at index 1: `for (int i = 1; i < NUM_LEDS; i++)`
- Use `DISPLAY_LEDS` (31) for height calculations
- Use `DISPLAY_LED_START` (1) as first display index

```cpp
// ✅ CORRECT - Skip LED 0
for (int i = 1; i < NUM_LEDS; i++) { 
  leds[i] = color; 
}

// ❌ WRONG - Includes LED 0 (level shift LED)
for (int i = 0; i < NUM_LEDS; i++) { 
  leds[i] = color; 
}
```

## Project Structure

| Directory | Purpose | Key Files | Status |
|-----------|---------|-----------|--------|
| `teensy_firmware/` | Arduino IDE firmware | `teensy_firmware.ino` (1681 lines) | **Production-ready** ⭐ |
| `firmware/teensy41/` | PlatformIO modular firmware | `src/main.cpp`, `src/pov_engine.cpp` | ~95% complete |
| `esp32_firmware/` | WiFi/BLE co-processor | `esp32_firmware.ino` | Production-ready |
| `esp32_firmware/webui/` | React web interface | `App.tsx`, `components/` | Production-ready |
| `examples/` | Python tools & tests | `image_converter.py`, `test_*.py` | Complete |
| `docs/` | Documentation | `API.md`, `WIRING.md`, 22 guides | Complete |

**Firmware recommendation**: Use `teensy_firmware/` (Arduino IDE) for production. PlatformIO version is for advanced users and some features are still being ported.

## Build Commands

### Firmware (C++ / Arduino)
```bash
# Teensy 4.1 firmware (Arduino IDE - recommended)
# 1. Open teensy_firmware/teensy_firmware.ino
# 2. Select Board: Teensy 4.1
# 3. Select USB Type: Serial
# 4. Click Upload

# PlatformIO builds
pio run -e teensy41           # Build Teensy firmware
pio run -e esp32              # Build ESP32 firmware
pio run -e esp32s3            # Build ESP32-S3 firmware (recommended)
pio run -e teensy41 -t upload # Upload Teensy
pio run -e esp32 -t upload    # Upload ESP32
```

### Web UI (React + TypeScript)
```bash
cd esp32_firmware/webui
npm install                   # Install dependencies
npm run dev                   # Development server on localhost:3000
npm run build                 # Production build to dist/
npm run preview               # Preview production build

# Build size: ~325KB optimized for ESP32 SPIFFS/LittleFS
```

### Python Tests
```bash
cd examples
pip install Pillow pytest     # Install dependencies
pytest test_*.py -v           # Run all tests
pytest test_image_converter.py -v  # Specific test
```

## REST API Quick Reference

Base URL: `http://192.168.4.1` (or `http://povpoi.local` via mDNS)

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/status` | GET | Returns `{mode, index, brightness, framerate, connected}` |
| `/api/mode` | POST | Set mode & index: `{"mode": 2, "index": 0}` |
| `/api/brightness` | POST | `{"brightness": 128}` (0-255) |
| `/api/framerate` | POST | `{"framerate": 60}` (10-120) |
| `/api/image` | POST | Multipart upload, auto-converts to 31px tall |
| `/api/pattern` | POST | `{"type": 0, "color1": "#FF0000", "speed": 50}` |
| `/api/live` | POST | Raw RGB frame for live mode |

## Serial Protocol (ESP32 ↔ Teensy)

Both protocols are fully implemented in current firmware:
- **Simple protocol**: `[0xFF][CMD][LEN][DATA...][0xFE]`
- **Structured protocol**: `[TYPE:1][LEN_H:1][LEN_L:1][DATA:LEN][CHECKSUM:1]`

Command codes in `teensy_firmware.ino`: 0x01=Mode, 0x02=Image, 0x03=Pattern, 0x05=LiveFrame, 0x06=Brightness, 0x07=FrameRate, 0x10=Status, 0x20-0x23=SD commands

Note: Dedicated SERIAL_PROTOCOL.md documentation is pending per review findings.

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

Music-reactive patterns (VU meter, pulse, rainbow, center, sparkle) are fully implemented and work with or without a microphone. The MAX9814 microphone module is **optional**:

**With microphone** (on Teensy pin A0):
```
MAX9814 VDD  → 3.3V
MAX9814 GND  → GND
MAX9814 OUT  → Teensy A0
MAX9814 GAIN → Leave floating (60dB) or GND (50dB)
```
**Without microphone**: Patterns will run but display low/zero audio levels.

Audio config in `teensy_firmware.ino`: `AUDIO_PIN A0`, `AUDIO_SAMPLES 64`, `AUDIO_NOISE_FLOOR 50`

## Image Conversion & Orientation

### Image Dimensions
- **HEIGHT**: 31 pixels (FIXED - one pixel per display LED)
- **WIDTH**: Variable (calculated from aspect ratio, max ~200px)
- LED strip forms the VERTICAL axis when spinning
- LED 1 (bottom of strip) = bottom of image
- LED 31 (top of strip) = top of image

### Image Storage Format
```cpp
// Storage: pixels[x][y] where y is LED index
CRGB pixels[IMAGE_WIDTH][IMAGE_HEIGHT];  // Max 31x200

// Display mapping (NO flip needed in latest code):
leds[y] = pixels[current_column][y];  // y ranges 1-31
```

### Python Converters
All converters must produce 31px tall images:
```python
# Resize maintaining aspect ratio
target_height = 31  # FIXED for 31 display LEDs
aspect_ratio = img.width / img.height
target_width = int(target_height * aspect_ratio)
img = img.resize((target_width, target_height), Image.LANCZOS)

# Note: Vertical flip handled by display logic, not converter
```

## Testing

### Python Tests
Python tests in `examples/` use pytest with Pillow-generated test images:
```bash
cd examples
pip install Pillow pytest
pytest test_*.py -v
```

**Key test files:**
- `test_image_converter.py` - Conversion, resizing, aspect ratio
- `test_vertical_flip.py` - POV orientation validation
- `test_error_handling.py` - Invalid inputs, missing files

**Common test failures:**
- Missing Pillow: `pip install Pillow`
- Wrong working directory: must run from `examples/`
- Image dimension assertions: check 31px height (fixed), variable width

### Manual Testing
```bash
# Teensy standalone test (no ESP32 required)
python test_teensy_standalone.py

# BLE protocol test
cd examples
python test_ble_protocol.py

# Serial debugging
python scripts/capture_serial_debug.py
```

## Common Development Tasks

### Adding New Patterns
Patterns defined in `displayPattern()` switch in `teensy_firmware.ino`. Types 0-17 exist.

```cpp
// In teensy_firmware.ino displayPattern()
case 18:  // New pattern ID
  for (int i = 1; i < NUM_LEDS; i++) {  // Start from 1!
    leds[i] = CHSV(hue + i * 8, 255, 255);
  }
  break;
```

**Steps:**
1. Add case to `displayPattern()` switch
2. Update `MAX_PATTERNS` constant
3. Add pattern button to ESP32 web interface
4. Test with various speeds (1-255)

### Modifying Web UI
```bash
cd esp32_firmware/webui
npm install          # First time only
npm run dev          # Start dev server with hot reload

# Make changes to:
# - App.tsx - Main app structure
# - components/ - Individual components
# - types.ts - TypeScript interfaces
# - constants.tsx - API URLs, config

npm run build        # Build for production
# Output: dist/ directory
```

### Image Conversion Tools
```bash
cd examples

# GUI converter (easiest)
python image_converter_gui.py

# CLI converter
python image_converter.py input.jpg
# Creates: input_31px.bin (binary), input_31px.png (preview)

# Web upload - automatic conversion
# Access http://192.168.4.1 and use upload button
```

## Troubleshooting

### No LED Output
1. Check power supply (5V, 2-3A minimum)
2. Verify APA102 connections:
   - Teensy Pin 11 → LED Data (DI)
   - Teensy Pin 13 → LED Clock (CI)
3. **LED 0 must be connected** (required for level shifting)
4. Test with simple code:
   ```cpp
   leds[1] = CRGB::Red;
   FastLED.show();
   ```

### WiFi Connection Issues
1. Verify ESP32 is powered and programmed
2. Check SSID/password in `esp32_firmware.ino`:
   - SSID: `POV-POI-WiFi`
   - Password: `povpoi123`
3. Try direct IP: `http://192.168.4.1`
4. Check ESP32 serial monitor for boot messages
5. mDNS alternative: `http://povpoi.local`

### Serial Communication Problems (Teensy ↔ ESP32)
1. Verify baud rate: **115200** on both devices
2. Check TX/RX connections (TX→RX, RX→TX):
   - Teensy TX1 (Pin 1) → ESP32 RX2 (GPIO 16)
   - Teensy RX1 (Pin 0) → ESP32 TX2 (GPIO 17)
3. Monitor Serial1 on Teensy for incoming commands
4. Check ESP32 serial output for command echo
5. Common ground between Teensy and ESP32 required

### Image Orientation Issues
- Images should display correctly without manual flipping
- LED 1 = bottom of strip = bottom of image
- LED 31 = top of strip = top of image
- If upside down, check physical LED strip orientation
- See `docs/POV_DISPLAY_ORIENTATION_GUIDE.md`

### Web UI Build Issues
```bash
# Clear and reinstall dependencies
cd esp32_firmware/webui
rm -rf node_modules package-lock.json
npm install

# Check Node version (18+ required)
node --version

# Verify Tailwind is configured (NOT using CDN)
# Check postcss.config.js and tailwind.config.js exist
```

### Memory Issues on Teensy
- Without PSRAM: Limit to ~10 images (32×200px)
- With PSRAM: Support up to ~50 images (32×400px)
- Reduce image width if running out of memory
- Consider enabling SD card support for external storage

## Key Constraints

- **Display modes**: 0=Idle, 1=Image, 2=Pattern, 3=Sequence, 4=Live
- **Ranges**: brightness 0-255, FPS 10-120, image dimensions W×31 (width variable, height fixed at 31px)
- **WiFi**: SSID `POV-POI-WiFi`, password `povpoi123`, IP `192.168.4.1`
- **Performance**: Teensy loop is time-critical - no blocking calls
- **Power**: Full brightness LEDs draw 2-3A at 5V

## Memory Considerations

### Teensy 4.1
- **Internal RAM**: 1MB total
- **Flash**: 8MB total
- **PSRAM (Optional)**: 16MB (2× 8MB chips soldered to board)
  - **With PSRAM**: ~1.8MB for 50 images (32×400 pixels each)
  - **Without PSRAM**: ~60KB for 10 images (32×200 pixels each)
  - PSRAM is 2-3x slower than internal RAM but sufficient for image storage
- **Pattern storage**: Minimal (~100 bytes per pattern)

### ESP32-S3 N16R8 (Recommended)
- **Flash**: 16MB (recommended for future expansion)
- **PSRAM**: 8MB (external RAM for web server buffering)
- **Web server**: Handles image upload/storage temporarily

### ESP32 (Standard)
- **Flash**: 4MB
- **PSRAM**: Usually none (some modules have 4MB)

## Security Best Practices

### API Keys & Secrets
- **NEVER** expose API keys in client-side code or Git
- Use empty string placeholders in `vite.config.ts` define block
- Implement backend proxy for external API calls
- Check for missing keys and show user-friendly errors

```typescript
// ✅ CORRECT - vite.config.ts
define: {
  'import.meta.env.VITE_API_KEY': '""',  // Empty placeholder
}

// ✅ CORRECT - Service code
const apiKey = import.meta.env.VITE_API_KEY;
if (!apiKey) {
  console.error('API key not configured');
  return;
}
```

### Arduino/ESP32
- WiFi credentials are in code (acceptable for this embedded system)
- No sensitive user data stored
- Local network only (AP mode), no internet connection required

## Performance Optimization

### ESP32 Firmware
- **Use ArduinoJson** library instead of String concatenation for JSON
  - Prevents heap fragmentation
  - Better performance
  - More reliable

```cpp
// ✅ CORRECT - Use ArduinoJson
DynamicJsonDocument doc(256);
doc["brightness"] = brightness;
doc["framerate"] = framerate;
serializeJson(doc, response);

// ❌ WRONG - String concatenation
String response = "{\"brightness\":" + String(brightness) + "}";
```

- **Cache calculated values** like `frameDelay = 1000/frameRate`
  - Store in struct instead of recalculating on every request
  
### Web UI
- **Debounce user inputs** (sliders, text fields) before API calls
- **Use refs for timer effects** to prevent unwanted re-renders
- **Import CSS in entry point** (`index.tsx`), not HTML
- Build produces **single-chunk bundle** (no code splitting for embedded systems)

## SD Card (Optional)

### Arduino IDE Firmware (`teensy_firmware/`)
- **Enable**: Uncomment `#define SD_SUPPORT` in `teensy_firmware.ino`
- **Directory**: `/poi_images/` (user must create manually)
- **Uses**: Teensy 4.1 built-in SD slot
- **Format**: FAT32 preferred (exFAT also supported via SdFat library)

### PlatformIO Firmware (`firmware/teensy41/`)
- **Always enabled** with SD support
- **Directory**: `/images/` (auto-created on first save)
- **SDIO**: Configured with `SdioConfig(FIFO_SDIO)` for performance (~20-25 MB/s read)

**Card format**: FAT32 preferred. Single partition, no scratch partition needed.

**Commands**: 0x20-0x23 (save/load/list/delete) when SD support enabled

## Additional Resources

- **Documentation**: See `docs/` directory (22 comprehensive guides)
- **FastLED Library**: https://github.com/FastLED/FastLED
- **Teensy 4.1**: https://www.pjrc.com/store/teensy41.html
- **APA102 Datasheet**: https://cdn-shop.adafruit.com/product-files/2343/APA102C.pdf
- **ESP32-S3 Docs**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/

## Recent Features

### Peer-to-Peer Synchronization
- Multiple poi can discover and sync with each other
- Bidirectional sharing of images, patterns, settings
- No master/slave - all devices are equal peers
- See `docs/POI_PAIRING.md` for setup

### BLE Support
- Nordic UART Service (NUS) for cross-platform compatibility
- Works with Windows and Web (Chrome/Edge browsers)
- Completely offline operation
- See `docs/BLE_PROTOCOL.md` for commands

## Quick Tips

1. **Always test at low brightness first** (&lt; 50) before ramping up
2. **Common ground is critical** - connects Teensy, ESP32, and LED power
3. **Use current-limited power supply** (2-3A minimum)
4. **Secure all connections** before spinning poi
5. **Check docs/** directory for detailed guides on specific topics
6. **CLAUDE.md** has comprehensive project context for AI assistants

## Style Conventions

### C++ Firmware
- **Functions**: `camelCase` (e.g., `displayPattern()`)
- **Constants**: `UPPER_CASE` (e.g., `NUM_LEDS`, `DISPLAY_LEDS`)
- **Variables**: `camelCase` (e.g., `currentPatternIndex`)
- **LED loops**: ALWAYS start from index 1
- **Comments**: Use for complex logic, match existing style
- **No blocking calls** in Teensy loop - it's time-critical

### Python
- Follow **PEP 8** style guide
- Use **type hints** for function signatures
- Use **Pillow** (PIL) for image processing
- Docstrings for public functions

### TypeScript / React (Web UI)
- **Components**: PascalCase (e.g., `Dashboard.tsx`)
- **Functions**: camelCase (e.g., `handleBrightnessChange`)
- **Constants**: UPPER_CASE (e.g., `API_BASE_URL`)
- **Hooks**: Use `use` prefix (e.g., `useDebounce`)
- **Types**: PascalCase interfaces/types in `types.ts`
- **Styling**: Tailwind CSS utility classes (NOT CDN, uses PostCSS)
- **State management**: React useState/useEffect
- **API calls**: Debounce user inputs (200-300ms) to reduce network traffic

### Web UI Best Practices
```typescript
// ✅ CORRECT - Debounce slider inputs
const debouncedBrightness = useDebounce(brightness, 300);
useEffect(() => {
  // API call with debounced value
}, [debouncedBrightness]);

// ✅ CORRECT - Use refs for timer/interval effects
const sequenceRef = useRef(sequence);
useEffect(() => {
  sequenceRef.current = sequence;
}, [sequence]);

useEffect(() => {
  const timer = setInterval(() => {
    // Access sequenceRef.current without adding sequence to deps
  }, 1000);
  return () => clearInterval(timer);
}, []); // Empty deps, no timer resets

// ✅ CORRECT - Import CSS in index.tsx for Vite
// In index.tsx:
import './styles.css';  // Not in HTML!
```

