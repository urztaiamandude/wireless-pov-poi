#!/bin/bash

# Quick Start Script for POV POI Android App
# This script helps you get started with the Android app

echo "=========================================="
echo "POV POI Android App - Quick Start"
echo "=========================================="
echo ""

# Check if Android Studio is installed (macOS)
if [[ "$OSTYPE" == "darwin"* ]]; then
    if [ -d "/Applications/Android Studio.app" ]; then
        echo "✓ Android Studio found on macOS"
        echo ""
        echo "Opening project in Android Studio..."
        open -a "Android Studio" POVPoiApp/
        exit 0
    fi
fi

# Check if Android Studio is installed (Linux)
if command -v studio &> /dev/null || command -v android-studio &> /dev/null; then
    echo "✓ Android Studio found on Linux"
    echo ""
    echo "Opening project in Android Studio..."
    if command -v studio &> /dev/null; then
        studio POVPoiApp/ &
    else
        android-studio POVPoiApp/ &
    fi
    exit 0
fi

# If Android Studio not found, provide instructions
echo "Android Studio not detected on your system."
echo ""
echo "To build the Android app:"
echo ""
echo "1. Install Android Studio from:"
echo "   https://developer.android.com/studio"
echo ""
echo "2. Open Android Studio"
echo ""
echo "3. Select 'Open an existing project'"
echo ""
echo "4. Navigate to and select the 'POVPoiApp' directory"
echo ""
echo "5. Wait for Gradle sync to complete"
echo ""
echo "6. Click 'Run' to build and install on your device"
echo ""
echo "=========================================="
echo "For detailed instructions, see:"
echo "  POVPoiApp/README.md"
echo "=========================================="
