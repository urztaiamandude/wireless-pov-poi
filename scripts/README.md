# Build Scripts

> Note: Build documentation has moved to the wiki.
> See [Build-Teensy-CLI](../wiki/Build-Teensy-CLI.md) and
> [Build-Teensy-HEX](../wiki/Build-Teensy-HEX.md).

This directory contains scripts for building the Teensy 4.1 firmware into HEX format for use with the Teensy Loader application.

## Available Scripts

### Arduino CLI Scripts (Recommended for CLI)

#### `build_arduino_cli.sh` (Linux/Mac)
Bash script to compile Teensy firmware using Arduino CLI.

**Usage:**
```bash
./scripts/build_arduino_cli.sh
```

**Prerequisites:**
- Arduino CLI: https://arduino.github.io/arduino-cli/
- Auto-installs Teensy platform and FastLED library on first run

**Output:**
- `teensy_firmware/build/teensy_firmware.ino.hex`

#### `build_arduino_cli.bat` (Windows)
Batch script to compile Teensy firmware using Arduino CLI.

**Usage:**
```cmd
scripts\build_arduino_cli.bat
```

**Prerequisites:**
- Arduino CLI: https://github.com/arduino/arduino-cli/releases
- Auto-installs Teensy platform and FastLED library on first run

**Output:**
- `teensy_firmware\build\teensy_firmware.ino.hex`

### PlatformIO Scripts (Advanced)

#### `build_teensy_hex.sh` (Linux/Mac)
Bash script to compile Teensy firmware to HEX format using PlatformIO.

**Usage:**
```bash
./scripts/build_teensy_hex.sh
```

**Prerequisites:**
- PlatformIO installed: `pip install platformio`

**Output:**
- `build_output/teensy41_firmware.hex`

#### `build_teensy_hex.bat` (Windows)
Batch script to compile Teensy firmware to HEX format using PlatformIO.

**Usage:**
```cmd
scripts\build_teensy_hex.bat
```

**Prerequisites:**
- PlatformIO installed: `pip install platformio`

**Output:**
- `build_output\teensy41_firmware.hex`

### Supporting Scripts

#### `post_build_teensy.py`
PlatformIO post-build script that automatically:
- Copies the generated HEX file to `build_output/teensy41_firmware.hex`
- Displays instructions for loading with Teensy Loader

This script is called automatically by PlatformIO during the build process.

## Quick Start

### For CLI Compilation (Easiest)

**Arduino CLI Method:**
```bash
# Install Arduino CLI first
# Linux: curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
# Mac: brew install arduino-cli
# Windows: winget install ArduinoSA.CLI

# Then build
./scripts/build_arduino_cli.sh      # Linux/Mac
scripts\build_arduino_cli.bat       # Windows
```

**PlatformIO Method:**
```bash
# Install PlatformIO first
pip install platformio

# Then build
./scripts/build_teensy_hex.sh       # Linux/Mac
scripts\build_teensy_hex.bat        # Windows
```

## Alternative: Arduino IDE Method

The easiest way to generate a HEX file is using Arduino IDE:

1. Open `teensy_firmware/teensy_firmware.ino` in Arduino IDE
2. Select **Tools > Board > Teensy 4.1**
3. Select **Tools > USB Type > Serial**
4. Go to **Sketch > Export Compiled Binary**
5. The HEX file will be created in the `teensy_firmware` directory

## Comparison

| Feature | Arduino CLI | PlatformIO | Arduino IDE |
|---------|-------------|------------|-------------|
| Setup | Auto-installs deps | Requires pip | Manual install |
| Speed | Fast | Faster (cached) | Medium |
| Automation | ✅ Yes | ✅ Yes | ❌ GUI only |
| Output | `teensy_firmware/build/` | `build_output/` | `teensy_firmware/` |
| Best For | CLI users | CI/CD, pros | Beginners |

## Troubleshooting

### Arduino CLI Issues

**"arduino-cli not found"**
- Install from: https://arduino.github.io/arduino-cli/
- Or use package manager (brew, winget)

**"Platform not found"**
- Script auto-installs on first run
- Or manually: `arduino-cli core install teensy:avr`

### PlatformIO Issues

**"pio not found"**
- Install: `pip install platformio`
- Verify: `pio --version`

**"Platform download failed"**
- Try: `pio run -e teensy41 --target clean`
- Check internet connection

### Arduino IDE Issues

**"Teensy board not found"**
- Install Teensyduino: https://www.pjrc.com/teensy/td_download.html
- Restart Arduino IDE

**"FastLED library not found"**
- Install via Library Manager: Sketch > Include Library > Manage Libraries

## More Information

For complete documentation on building and loading HEX files, see:
- **[CLI Compilation Guide](../docs/CLI_COMPILATION.md)** - Complete CLI reference
- **[Building HEX Files Guide](../docs/BUILDING_HEX.md)** - All methods
- **[Main README](../README.md)** - Project overview
