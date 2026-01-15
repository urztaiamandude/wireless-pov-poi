# CLI Compilation Guide for Teensy 4.1 Firmware

Complete guide for compiling Teensy firmware from the command line.

## Overview

Two CLI options are available:
1. **Arduino CLI** - Official Arduino command-line interface (recommended for Arduino users)
2. **PlatformIO CLI** - Professional build system (recommended for automation/CI/CD)

---

## Method 1: Arduino CLI

### Installation

**Linux:**
```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
```

**macOS:**
```bash
brew install arduino-cli
```

**Windows:**
```powershell
# Download from https://github.com/arduino/arduino-cli/releases
# Or use winget:
winget install ArduinoSA.CLI
```

**Verify Installation:**
```bash
arduino-cli version
```

### Initial Setup

```bash
# Initialize configuration
arduino-cli config init

# Update the index
arduino-cli core update-index

# Add Teensy board support URL to config
arduino-cli config add board_manager.additional_urls https://www.pjrc.com/teensy/package_teensy_index.json

# Update index again to include Teensy
arduino-cli core update-index

# Install Teensy platform
arduino-cli core install teensy:avr

# Install FastLED library
arduino-cli lib install FastLED
```

### Compile to HEX

```bash
# Navigate to project root
cd /path/to/wireless-pov-poi

# Compile the firmware
arduino-cli compile \
  --fqbn teensy:avr:teensy41:usb=serial,speed=600,opt=o2std \
  teensy_firmware/teensy_firmware.ino \
  --output-dir teensy_firmware/build

# The HEX file will be at:
# teensy_firmware/build/teensy_firmware.ino.hex
```

### One-Line Compile Command

```bash
arduino-cli compile --fqbn teensy:avr:teensy41:usb=serial,speed=600,opt=o2std teensy_firmware/teensy_firmware.ino --output-dir teensy_firmware/build
```

### Build Script for Arduino CLI

Create `scripts/build_arduino_cli.sh`:

```bash
#!/bin/bash
set -e

echo "Building with Arduino CLI..."

# Check if arduino-cli is installed
if ! command -v arduino-cli &> /dev/null; then
    echo "Error: arduino-cli is not installed"
    echo "Install from: https://arduino.github.io/arduino-cli/latest/installation/"
    exit 1
fi

# Compile
arduino-cli compile \
  --fqbn teensy:avr:teensy41:usb=serial,speed=600,opt=o2std \
  teensy_firmware/teensy_firmware.ino \
  --output-dir teensy_firmware/build

echo ""
echo "✓ Build complete!"
echo "HEX file: teensy_firmware/build/teensy_firmware.ino.hex"
```

### Upload Directly via CLI

```bash
# Upload to connected Teensy board
arduino-cli upload \
  --fqbn teensy:avr:teensy41 \
  --port /dev/ttyACM0 \
  teensy_firmware/teensy_firmware.ino

# On Windows, use COM port:
# arduino-cli upload --fqbn teensy:avr:teensy41 --port COM3 teensy_firmware/teensy_firmware.ino
```

---

## Method 2: PlatformIO CLI

### Installation

```bash
# Install via pip
pip install platformio

# Or using pipx (recommended for isolated install)
pipx install platformio

# Verify
pio --version
```

### First-Time Setup

```bash
# Navigate to project root
cd /path/to/wireless-pov-poi

# Install Teensy platform (first time only)
pio platform install teensy

# Install dependencies (first time only)
pio lib install
```

### Compile to HEX

```bash
# Clean previous build
pio run -e teensy41 --target clean

# Build firmware
pio run -e teensy41

# HEX file locations:
# 1. .pio/build/teensy41/firmware.hex
# 2. build_output/teensy41_firmware.hex (copied by post-build script)
```

### Using Build Scripts

**Linux/Mac:**
```bash
./scripts/build_teensy_hex.sh
```

**Windows:**
```cmd
scripts\build_teensy_hex.bat
```

### Upload Directly via CLI

```bash
# Upload to connected Teensy
pio run -e teensy41 --target upload

# Or specify port
pio run -e teensy41 --target upload --upload-port /dev/ttyACM0
```

### Verbose Output

```bash
# For debugging compilation issues
pio run -e teensy41 --verbose
```

---

## Comparison: Arduino CLI vs PlatformIO

| Feature | Arduino CLI | PlatformIO |
|---------|-------------|------------|
| **Installation** | Single binary | Python package |
| **Setup Time** | 5 minutes | 3 minutes |
| **Build Speed** | Fast | Faster (cached) |
| **Library Management** | arduino-cli lib | platformio.ini |
| **CI/CD Ready** | ✅ Yes | ✅ Yes |
| **IDE Integration** | Arduino IDE | VSCode, others |
| **Configuration** | Command flags | platformio.ini file |
| **Best For** | Arduino users | Professional dev |

---

## CI/CD Integration

### GitHub Actions (Arduino CLI)

```yaml
name: Build Teensy Firmware
on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Install Arduino CLI
        run: |
          curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
          echo "$HOME/bin" >> $GITHUB_PATH
      
      - name: Setup Arduino CLI
        run: |
          arduino-cli config init
          arduino-cli config add board_manager.additional_urls https://www.pjrc.com/teensy/package_teensy_index.json
          arduino-cli core update-index
          arduino-cli core install teensy:avr
          arduino-cli lib install FastLED
      
      - name: Compile Firmware
        run: |
          arduino-cli compile \
            --fqbn teensy:avr:teensy41:usb=serial,speed=600,opt=o2std \
            teensy_firmware/teensy_firmware.ino \
            --output-dir teensy_firmware/build
      
      - name: Upload Artifact
        uses: actions/upload-artifact@v4
        with:
          name: teensy-firmware-hex
          path: teensy_firmware/build/teensy_firmware.ino.hex
```

### GitHub Actions (PlatformIO)

```yaml
name: Build Teensy Firmware
on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: '3.x'
      
      - name: Install PlatformIO
        run: pip install platformio
      
      - name: Build Firmware
        run: pio run -e teensy41
      
      - name: Upload Artifact
        uses: actions/upload-artifact@v4
        with:
          name: teensy-firmware-hex
          path: build_output/teensy41_firmware.hex
```

---

## Troubleshooting

### Arduino CLI Issues

**"Platform not found"**
```bash
arduino-cli core update-index
arduino-cli core install teensy:avr
```

**"Library not found"**
```bash
arduino-cli lib search FastLED
arduino-cli lib install FastLED
```

**"Board not recognized"**
```bash
# List available boards
arduino-cli board listall teensy

# Verify FQBN
arduino-cli board details -b teensy:avr:teensy41
```

**"Compilation failed"**
```bash
# Compile with verbose output
arduino-cli compile --verbose --fqbn teensy:avr:teensy41 teensy_firmware/teensy_firmware.ino
```

### PlatformIO Issues

**"Platform not installed"**
```bash
pio platform install teensy
```

**"Library dependencies failed"**
```bash
pio lib install
# Or force reinstall
pio lib install --force
```

**"Upload failed"**
```bash
# List connected devices
pio device list

# Specify port manually
pio run -e teensy41 --target upload --upload-port /dev/ttyACM0
```

---

## Quick Reference

### Arduino CLI Quick Commands

```bash
# Compile only
arduino-cli compile --fqbn teensy:avr:teensy41:usb=serial,speed=600,opt=o2std teensy_firmware/teensy_firmware.ino

# Compile and upload
arduino-cli compile --fqbn teensy:avr:teensy41:usb=serial,speed=600,opt=o2std teensy_firmware/teensy_firmware.ino && \
arduino-cli upload --fqbn teensy:avr:teensy41 --port /dev/ttyACM0 teensy_firmware/teensy_firmware.ino

# List boards
arduino-cli board list

# Update everything
arduino-cli core update-index && arduino-cli lib update-index
```

### PlatformIO Quick Commands

```bash
# Clean + Build
pio run -e teensy41 --target clean && pio run -e teensy41

# Build + Upload
pio run -e teensy41 --target upload

# Monitor serial output
pio device monitor --port /dev/ttyACM0 --baud 115200

# List devices
pio device list
```

---

## Loading HEX Files

### Using Teensy Loader (GUI)

1. Open Teensy Loader application
2. File > Open HEX File
3. Select compiled HEX file
4. Press button on Teensy board
5. Auto-programs

### Using Teensy Loader (CLI)

```bash
# Install teensy_loader_cli
# Source: https://github.com/PaulStoffregen/teensy_loader_cli

# Linux/Mac compilation:
git clone https://github.com/PaulStoffregen/teensy_loader_cli.git
cd teensy_loader_cli
make

# Upload HEX file
./teensy_loader_cli --mcu=TEENSY41 -w -v teensy_firmware.hex
```

### Using TyTools (Alternative CLI)

```bash
# Install TyTools
# Linux: sudo apt install tytools
# Mac: brew install tytools

# Upload HEX
tycommander upload teensy_firmware.hex
```

---

## Recommended Workflow

### For Development
```bash
# Use PlatformIO for fast iteration
pio run -e teensy41 --target upload
pio device monitor --baud 115200
```

### For Distribution
```bash
# Use Arduino CLI for standard builds
arduino-cli compile --fqbn teensy:avr:teensy41:usb=serial,speed=600,opt=o2std teensy_firmware/teensy_firmware.ino
# Distribute the HEX file
```

### For CI/CD
```bash
# Use PlatformIO for automated builds
pio run -e teensy41
# Upload artifacts
```

---

## Additional Resources

- **Arduino CLI Documentation**: https://arduino.github.io/arduino-cli/
- **PlatformIO Teensy**: https://docs.platformio.org/en/latest/platforms/teensy.html
- **Teensy Documentation**: https://www.pjrc.com/teensy/
- **Teensy Loader CLI**: https://github.com/PaulStoffregen/teensy_loader_cli
- **TyTools**: https://github.com/Koromix/tytools

---

## Summary

**Choose Arduino CLI if you:**
- Are familiar with Arduino ecosystem
- Want official Arduino tooling
- Need simple setup with minimal dependencies

**Choose PlatformIO if you:**
- Want advanced build features
- Need library dependency management
- Are building CI/CD pipelines
- Want IDE integration (VSCode)

Both methods produce identical HEX files that work with Teensy Loader!
