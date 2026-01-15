#!/bin/bash
# Build script for compiling Teensy 4.1 firmware to HEX format
# This HEX file can be loaded using the Teensy Loader application

set -e  # Exit on error

echo "======================================================"
echo "Building Teensy 4.1 Firmware for Teensy Loader"
echo "======================================================"
echo ""

# Check if PlatformIO is installed
if ! command -v pio &> /dev/null; then
    echo "Error: PlatformIO is not installed!"
    echo ""
    echo "To install PlatformIO:"
    echo "  pip install platformio"
    echo ""
    echo "Or install via the PlatformIO IDE:"
    echo "  https://platformio.org/install/ide"
    exit 1
fi

echo "✓ PlatformIO found: $(pio --version)"
echo ""

# Clean previous build
echo "Cleaning previous build..."
pio run -e teensy41 --target clean

echo ""
echo "Building firmware..."
echo ""

# Build the firmware
pio run -e teensy41

echo ""
echo "======================================================"
echo "Build Complete!"
echo "======================================================"
echo ""
echo "The HEX file is ready to load with Teensy Loader:"
echo "  Location: build_output/teensy41_firmware.hex"
echo ""
echo "To upload using Teensy Loader:"
echo "  1. Open Teensy Loader application"
echo "  2. File > Open HEX File"
echo "  3. Select: build_output/teensy41_firmware.hex"
echo "  4. Press the button on your Teensy board"
echo "  5. Click 'Program' in Teensy Loader"
echo ""
echo "======================================================"
