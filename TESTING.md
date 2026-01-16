# Testing Environment and Tools

This document describes the testing tools, environment setup, and procedures used for the Nebula POV Poi project.

## Overview

The project uses different testing approaches for different components:

- **Python Scripts**: pytest with Pillow for image conversion tools
- **C++ Firmware**: PlatformIO static analysis with cppcheck and clangtidy
- **Hardware Integration**: Manual testing with physical devices

## Platform Support

### Operating Systems

All testing tools support **multiple operating systems**:

| Platform | Python Tests | Firmware Build | Hardware Testing |
|----------|-------------|----------------|------------------|
| **Linux** | ✅ Fully supported | ✅ Fully supported | ✅ Fully supported |
| **macOS** | ✅ Fully supported | ✅ Fully supported | ✅ Fully supported |
| **Windows** | ✅ Fully supported | ✅ Fully supported | ✅ Fully supported |

**Development Environment Used:**
- Primary development and CI testing: **Linux (Ubuntu 20.04+)**
- Fully tested on: Windows 10/11, macOS 10.15+
- All commands work cross-platform with minor syntax adjustments

### Virtual Environment

**Python Virtual Environment - RECOMMENDED**

Using a Python virtual environment is **strongly recommended** but not required:

**Why use a virtual environment?**
- ✅ Isolates project dependencies from system Python
- ✅ Prevents version conflicts with other projects
- ✅ Easy to reproduce exact testing environment
- ✅ Safe to experiment without affecting system

**Setup (recommended):**

```bash
# Linux/macOS
cd wireless-pov-poi
python3 -m venv venv
source venv/bin/activate

# Windows
cd wireless-pov-poi
python -m venv venv
venv\Scripts\activate

# Install dependencies in virtual environment
pip install -r examples/requirements.txt
pip install pytest
```

**Without virtual environment:**

You can also install dependencies globally (not recommended for production):

```bash
# Linux/macOS (may require sudo)
pip3 install --user Pillow pytest

# Windows
pip install Pillow pytest
```

**Verify your environment:**

```bash
python --version  # Should be 3.7+
pip list | grep -i pillow  # Should show Pillow installed
pytest --version  # Should show pytest installed
```

## Python Testing Environment

### Required Tools

| Tool | Version | Purpose |
|------|---------|---------|
| Python | 3.7+ | Core interpreter |
| pip | Latest | Package manager |
| Pillow | ≥9.0.0 | Image processing library |
| pytest | Latest | Test framework |

### Installation

Install Python dependencies for testing:

```bash
cd examples
pip install -r requirements.txt
pip install pytest
```

Or install individually:

```bash
pip install Pillow pytest
```

### Running Python Tests

The `examples/` directory contains comprehensive test suites for the image conversion tools:

**Run all tests:**
```bash
cd examples
pytest test_*.py -v
```

**Run specific test file:**
```bash
cd examples
pytest test_image_converter.py -v
```

**Run with detailed output:**
```bash
cd examples
pytest test_*.py -v --tb=short
```

### Python Test Files

| File | Purpose | Tests |
|------|---------|-------|
| `test_image_converter.py` | Core conversion functionality | 8 test cases |
| `test_vertical_flip.py` | POV orientation validation | Vertical flip verification |
| `test_error_handling.py` | Error handling and validation | Missing dependencies, invalid inputs |
| `test_image_converter_gui.py` | GUI functionality | Interface and conversion tests |
| `test_installer_build.py` | Windows installer build | PyInstaller configuration |

### Test Coverage

The Python test suite validates:

- ✅ Image resizing to 31 pixels wide
- ✅ Aspect ratio preservation
- ✅ Height limiting (max 64 pixels)
- ✅ RGB color mode conversion
- ✅ Multiple image formats (PNG, JPEG, GIF)
- ✅ Vertical flip for POV display
- ✅ Error handling for invalid inputs
- ✅ Default output naming conventions

### Manual Testing

For GUI testing:
```bash
cd examples
python image_converter_gui.py
```

For command-line converter:
```bash
cd examples
python image_converter.py demo_arrow.png
```

## C++ Firmware Testing

### PlatformIO Static Analysis

The project uses PlatformIO's built-in static analysis tools:

| Tool | Purpose | Configuration |
|------|---------|---------------|
| cppcheck | C++ static analyzer | All checks enabled |
| clangtidy | LLVM-based linter | Analyzer and performance checks |

### Configuration

Defined in `platformio.ini`:

```ini
[env]
check_tool = cppcheck, clangtidy
check_flags = 
    cppcheck: --enable=all
    clangtidy: --checks=-*,clang-analyzer-*,performance-*
```

### Running Static Analysis

**Check Teensy firmware:**
```bash
pio check -e teensy41
```

**Check ESP32 firmware:**
```bash
pio check -e esp32
```

**Check all environments:**
```bash
pio check
```

### Build Verification

**Build Teensy firmware:**
```bash
pio run -e teensy41
```

**Build ESP32 firmware:**
```bash
pio run -e esp32
```

**Build both:**
```bash
pio run
```

### Compilation Flags

The firmware uses optimized compilation settings:

**Teensy 4.1:**
```ini
build_flags = 
    -D USB_SERIAL
    -O2
```

**ESP32:**
```ini
build_flags = 
    -D CORE_DEBUG_LEVEL=3
```

## Hardware Testing

### Requirements

Hardware testing requires physical devices:

- Teensy 4.1 board
- ESP32 board (ESP32-DevKitC or similar)
- APA102 LED strip (32 LEDs)
- 5V power supply (2-3A)
- USB cables for programming
- Optional: MAX9814 microphone module for audio patterns

### Test Setup

1. **Wire hardware** according to [docs/WIRING.md](docs/WIRING.md)
2. **Upload firmware** to both Teensy and ESP32
3. **Power on** the system
4. **Connect** to WiFi network `POV-POI-WiFi` (password: `povpoi123`)
5. **Access** web interface at `http://192.168.4.1`

### Hardware Test Checklist

#### Display Modes
- [ ] Mode 0: Idle (LEDs off)
- [ ] Mode 1: Image display
- [ ] Mode 2: Pattern display (types 0-15)
- [ ] Mode 3: Sequence playback
- [ ] Mode 4: Live mode

#### Patterns (Mode 2)
- [ ] 0: Rainbow
- [ ] 1: Wave
- [ ] 2: Gradient
- [ ] 3: Sparkle
- [ ] 4: Fire
- [ ] 5: Comet
- [ ] 6: Solid Color
- [ ] 7: Theater Chase
- [ ] 8: Twinkle
- [ ] 9: Scanner
- [ ] 10: Strobe
- [ ] 11: Music VU Meter (requires microphone)
- [ ] 12: Music Pulse (requires microphone)
- [ ] 13: Audio Rainbow (requires microphone)
- [ ] 14: Music Wave (requires microphone)
- [ ] 15: Music Sparkle (requires microphone)

#### Web Interface
- [ ] Home page loads
- [ ] Pattern selection
- [ ] Image upload
- [ ] Brightness control (0-255)
- [ ] Frame rate control (10-120)
- [ ] Live drawing mode
- [ ] Mobile responsive design

#### REST API Endpoints
- [ ] GET `/api/status`
- [ ] POST `/api/mode`
- [ ] POST `/api/brightness`
- [ ] POST `/api/framerate`
- [ ] POST `/api/image`
- [ ] POST `/api/pattern`
- [ ] POST `/api/live`

#### Serial Communication
- [ ] ESP32 → Teensy commands
- [ ] Status responses
- [ ] Error handling
- [ ] Baud rate: 115200

#### Power and Performance
- [ ] Power consumption at full brightness
- [ ] Battery runtime estimation
- [ ] LED refresh rate
- [ ] Response time to commands
- [ ] WiFi stability

### Test Procedures

#### Pattern Test
1. Select pattern mode
2. Choose pattern type
3. Adjust speed setting
4. Verify LED animation
5. Check color accuracy

#### Image Upload Test
1. Prepare test image (any size)
2. Upload via web interface
3. Verify automatic conversion to 31×64
4. Display image in POV mode
5. Check vertical flip orientation

#### Brightness Test
1. Set brightness to 0 (should be off)
2. Gradually increase to 255
3. Verify smooth transition
4. Check power consumption
5. Test at different ambient lighting

#### Frame Rate Test
1. Set frame rate to 10 FPS
2. Observe POV effect
3. Increase to 60 FPS
4. Test up to 120 FPS
5. Find optimal rate for smooth display

## Development Environment Setup

### Required Software

#### For Firmware Development

**Option 1: Arduino IDE (Recommended)**
- Arduino IDE 1.8.x or 2.x
- Teensyduino addon
- ESP32 board support
- FastLED library

**Installation:**
1. Download Arduino IDE from https://www.arduino.cc/en/software
2. Install Teensyduino from https://www.pjrc.com/teensy/td_download.html
3. Add ESP32 board support via Board Manager
4. Install FastLED library via Library Manager

**Option 2: PlatformIO (Advanced)**
- PlatformIO Core or IDE
- VS Code (recommended IDE)
- Platform definitions handled automatically

**Installation:**
```bash
# Via pip
pip install platformio

# Or via VS Code extension
# Install "PlatformIO IDE" extension
```

#### For Python Development

- Python 3.7+
- pip package manager
- Virtual environment (recommended - see [Platform Support](#platform-support) section above)

### IDE Recommendations

| IDE | Best For | Features |
|-----|----------|----------|
| Arduino IDE | Quick firmware edits | Simple, Teensyduino integration |
| VS Code + PlatformIO | Professional development | IntelliSense, debugging, git integration |
| PyCharm | Python development | Advanced Python features, testing tools |
| VS Code + Python | General Python work | Lightweight, good extensions |

### Serial Monitor Tools

For debugging and monitoring:

- **Arduino IDE Serial Monitor** (Tools → Serial Monitor)
- **PlatformIO Serial Monitor** (`pio device monitor`)
- **Screen** (Linux/Mac): `screen /dev/ttyACM0 115200`
- **PuTTY** (Windows): Configure for COM port at 115200 baud
- **CoolTerm**: Cross-platform serial terminal

## Continuous Integration

### Current Status

The project currently uses **manual testing** and does not have automated CI/CD pipelines.

### Future CI/CD Plans

Potential GitHub Actions workflows:

1. **Python Tests**
   - Run pytest on all Python test files
   - Check code formatting with black
   - Validate requirements.txt

2. **Firmware Compilation**
   - Build Teensy firmware with PlatformIO
   - Build ESP32 firmware with PlatformIO
   - Check for compilation warnings
   - Run static analysis (cppcheck, clangtidy)

3. **Documentation**
   - Check markdown formatting
   - Validate links
   - Generate API docs

### Example GitHub Actions Workflow (Future)

```yaml
name: Tests

on: [push, pull_request]

jobs:
  python-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-python@v4
        with:
          python-version: '3.11'
      - name: Install dependencies
        run: |
          pip install pytest Pillow
      - name: Run tests
        run: |
          cd examples
          pytest test_*.py -v
  
  firmware-build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Set up PlatformIO
        run: pip install platformio
      - name: Build Teensy firmware
        run: pio run -e teensy41
      - name: Build ESP32 firmware
        run: pio run -e esp32
      - name: Run static analysis
        run: pio check
```

## Testing Best Practices

### Before Committing

1. ✅ Run Python tests: `cd examples && pytest test_*.py -v`
2. ✅ Build firmware: `pio run -e teensy41 -e esp32`
3. ✅ Check code quality: `pio check`
4. ✅ Test on hardware (if firmware changes)
5. ✅ Update documentation

### For Pull Requests

1. ✅ All tests pass
2. ✅ Code compiles without warnings
3. ✅ Static analysis passes
4. ✅ Hardware tested (for firmware changes)
5. ✅ Documentation updated
6. ✅ Commit messages are clear

### Common Issues

#### Python Tests Fail

**Issue:** `ModuleNotFoundError: No module named 'PIL'`

**Solution:**
```bash
pip install Pillow
```

**Issue:** Tests fail with image size errors

**Solution:** Check that images are being resized to 31 pixels wide (DISPLAY_LEDS constant)

#### Firmware Build Fails

**Issue:** `FastLED.h: No such file or directory`

**Solution:**
```bash
# PlatformIO
pio lib install "fastled/FastLED@^3.5.0"

# Arduino IDE
# Install via Library Manager
```

**Issue:** Teensy board not found

**Solution:** Install Teensyduino addon for Arduino IDE

#### Hardware Tests Fail

**Issue:** LEDs don't light up

**Solution:** Check power supply, wiring, and LED strip orientation

**Issue:** Web interface not accessible

**Solution:** 
- Verify WiFi connection to `POV-POI-WiFi`
- Check ESP32 serial monitor for IP address
- Try `http://povpoi.local` or `http://192.168.4.1`

## Resources

### Documentation

- [README.md](README.md) - Project overview
- [QUICKSTART.md](QUICKSTART.md) - Quick setup guide
- [docs/WIRING.md](docs/WIRING.md) - Hardware wiring
- [docs/API.md](docs/API.md) - REST API reference
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines

### External Resources

- [PlatformIO Documentation](https://docs.platformio.org/)
- [pytest Documentation](https://docs.pytest.org/)
- [FastLED Documentation](https://github.com/FastLED/FastLED)
- [Pillow Documentation](https://pillow.readthedocs.io/)
- [Arduino IDE Guide](https://www.arduino.cc/en/Guide)
- [Teensyduino Documentation](https://www.pjrc.com/teensy/teensyduino.html)

## Summary

### Quick Reference

**Python tests:**
```bash
cd examples && pytest test_*.py -v
```

**Build firmware:**
```bash
pio run -e teensy41 -e esp32
```

**Static analysis:**
```bash
pio check
```

**Install dependencies:**
```bash
pip install Pillow pytest
```

### Test Command Cheatsheet

| Command | Purpose |
|---------|---------|
| `pytest test_*.py -v` | Run all Python tests |
| `pytest -k test_name` | Run specific test |
| `pio run -e teensy41` | Build Teensy firmware |
| `pio run -e esp32` | Build ESP32 firmware |
| `pio check` | Run static analysis |
| `pio device monitor` | Open serial monitor |
| `python image_converter_gui.py` | Test GUI converter |

---

**Questions or issues?** Open an issue on GitHub or check [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.
