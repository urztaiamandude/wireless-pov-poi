#!/bin/bash
# Verification script for Nebula Poi setup
# This script checks that all necessary components are installed

echo "=========================================="
echo "Nebula Poi Setup Verification"
echo "=========================================="
echo ""

ERRORS=0

# Check if Arduino CLI is available (optional)
echo "Checking for Arduino CLI..."
if command -v arduino-cli &> /dev/null; then
    echo "✓ Arduino CLI found: $(arduino-cli version | head -n1)"
else
    echo "⚠ Arduino CLI not found (optional, GUI Arduino IDE is fine)"
fi
echo ""

# Check Python installation
echo "Checking Python installation..."
if command -v python3 &> /dev/null; then
    PYTHON_VERSION=$(python3 --version)
    echo "✓ Python found: $PYTHON_VERSION"
else
    echo "✗ Python 3 not found"
    echo "  Install Python 3 to use image converter script"
    ERRORS=$((ERRORS + 1))
fi
echo ""

# Check for PIL/Pillow (for image converter)
echo "Checking for PIL/Pillow (image processing)..."
if python3 -c "import PIL" 2>/dev/null; then
    echo "✓ PIL/Pillow installed"
else
    echo "⚠ PIL/Pillow not found (optional, needed for image converter)"
    echo "  Install with: pip3 install Pillow"
fi
echo ""

# Check directory structure
echo "Checking project structure..."
REQUIRED_DIRS=(
    "teensy_firmware"
    "esp32_firmware"
    "docs"
    "examples"
)

for dir in "${REQUIRED_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo "✓ Directory exists: $dir"
    else
        echo "✗ Missing directory: $dir"
        ERRORS=$((ERRORS + 1))
    fi
done
echo ""

# Check required files
echo "Checking required files..."
REQUIRED_FILES=(
    "teensy_firmware/teensy_firmware.ino"
    "esp32_firmware/esp32_firmware.ino"
    "README.md"
    "QUICKSTART.md"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ File exists: $file"
    else
        echo "✗ Missing file: $file"
        ERRORS=$((ERRORS + 1))
    fi
done
echo ""

# Check USB devices (Linux only)
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Checking for USB devices..."
    if command -v lsusb &> /dev/null; then
        if lsusb | grep -i "teensy" &> /dev/null; then
            echo "✓ Teensy device detected"
        else
            echo "⚠ No Teensy device detected (may not be connected)"
        fi
        
        if lsusb | grep -i "esp32\|CP210\|CH340" &> /dev/null; then
            echo "✓ ESP32/Serial device detected"
        else
            echo "⚠ No ESP32 device detected (may not be connected)"
        fi
    else
        echo "⚠ lsusb not available, cannot check USB devices"
    fi
    echo ""
fi

# Check for serial ports (if available)
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Checking serial ports..."
    if ls /dev/ttyACM* &> /dev/null || ls /dev/ttyUSB* &> /dev/null; then
        echo "Available serial ports:"
        ls -1 /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | while read port; do
            echo "  - $port"
        done
    else
        echo "⚠ No serial ports found"
    fi
    echo ""
fi

# Summary
echo "=========================================="
echo "Verification Summary"
echo "=========================================="

if [ $ERRORS -eq 0 ]; then
    echo "✓ All required components present!"
    echo ""
    echo "Next steps:"
    echo "1. Connect Teensy 4.1 via USB"
    echo "2. Open teensy_firmware/teensy_firmware.ino in Arduino IDE"
    echo "3. Select Tools > Board > Teensy 4.1"
    echo "4. Upload the firmware"
    echo "5. Repeat for ESP32 with esp32_firmware/esp32_firmware.ino"
    echo ""
    echo "See QUICKSTART.md for detailed instructions"
else
    echo "✗ Found $ERRORS error(s)"
    echo "Please resolve the issues above before proceeding"
fi

exit $ERRORS
