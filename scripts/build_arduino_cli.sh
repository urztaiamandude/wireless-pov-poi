#!/bin/bash
# Build script for compiling Teensy 4.1 firmware using Arduino CLI
# Generates HEX file for Teensy Loader

set -e  # Exit on error

# Configuration
TEENSY_FQBN="teensy:avr:teensy41:usb=serial,speed=600,opt=o2std"
SOURCE_FILE="teensy_firmware/teensy_firmware.ino"
OUTPUT_DIR="teensy_firmware/build"

echo "======================================================"
echo "Building Teensy 4.1 Firmware with Arduino CLI"
echo "======================================================"
echo ""

# Check if arduino-cli is installed
if ! command -v arduino-cli &> /dev/null; then
    echo "Error: arduino-cli is not installed!"
    echo ""
    echo "To install Arduino CLI:"
    echo "  Linux:"
    echo "    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh"
    echo ""
    echo "  macOS:"
    echo "    brew install arduino-cli"
    echo ""
    echo "  Windows:"
    echo "    winget install ArduinoSA.CLI"
    echo ""
    echo "  Or download from:"
    echo "    https://github.com/arduino/arduino-cli/releases"
    exit 1
fi

echo "✓ Arduino CLI found: $(arduino-cli version)"
echo ""

# Check if Teensy platform is installed
echo "Checking Teensy platform..."
if ! arduino-cli core list | grep -q "teensy:avr"; then
    echo "⚠️  Teensy platform not found. Installing..."
    arduino-cli config add board_manager.additional_urls https://www.pjrc.com/teensy/package_teensy_index.json
    arduino-cli core update-index
    arduino-cli core install teensy:avr
    echo "✓ Teensy platform installed"
else
    echo "✓ Teensy platform found"
fi
echo ""

# Check if FastLED library is installed
echo "Checking FastLED library..."
if ! arduino-cli lib list | grep -q "FastLED"; then
    echo "⚠️  FastLED library not found. Installing..."
    arduino-cli lib install FastLED
    echo "✓ FastLED library installed"
else
    echo "✓ FastLED library found"
fi
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo "Compiling firmware..."
echo ""

# Compile the firmware
arduino-cli compile \
  --fqbn "$TEENSY_FQBN" \
  "$SOURCE_FILE" \
  --output-dir "$OUTPUT_DIR"

echo ""
echo "======================================================"
echo "Build Complete!"
echo "======================================================"
echo ""
echo "The HEX file is ready to load with Teensy Loader:"
echo "  Location: teensy_firmware/build/teensy_firmware.ino.hex"
echo ""
echo "To upload using Teensy Loader:"
echo "  1. Open Teensy Loader application"
echo "  2. File > Open HEX File"
echo "  3. Select: teensy_firmware/build/teensy_firmware.ino.hex"
echo "  4. Press the button on your Teensy board"
echo "  5. Click 'Program' in Teensy Loader"
echo ""
echo "Or upload directly via CLI:"
echo "  arduino-cli upload --fqbn teensy:avr:teensy41 --port /dev/ttyACM0 teensy_firmware/teensy_firmware.ino"
echo ""
echo "======================================================"
