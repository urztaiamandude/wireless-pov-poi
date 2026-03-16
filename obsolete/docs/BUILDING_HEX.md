# Building HEX Files for Teensy Loader

> Note: Build documentation has moved to the wiki.
> See [Build-Teensy-HEX](../wiki/Build-Teensy-HEX.md) and
> [Build-Teensy-CLI](../wiki/Build-Teensy-CLI.md).

This guide explains how to compile the Teensy 4.1 firmware into a HEX file format that can be loaded using the Teensy Loader application.

## Prerequisites

You need one of the following:

### Option 1: Arduino IDE with Teensyduino (Recommended)
- Arduino IDE 1.8.x or 2.x
- Teensyduino add-on installed from https://www.pjrc.com/teensy/td_download.html
- FastLED library (install via Library Manager)

### Option 2: PlatformIO
- PlatformIO Core or PlatformIO IDE
- Install via: `pip install platformio`

## Building with Arduino IDE (Easiest Method)

### Step 1: Setup
1. Open Arduino IDE
2. Open `teensy_firmware/teensy_firmware.ino`
3. Select **Tools > Board > Teensy 4.1**
4. Select **Tools > USB Type > Serial**
5. Select **Tools > CPU Speed > 600 MHz**

### Step 2: Compile to HEX
1. Go to **Sketch > Export Compiled Binary** (or press Ctrl+Alt+S / Cmd+Option+S)
2. Wait for compilation to complete
3. The HEX file will be created in the `teensy_firmware` directory
4. Look for: `teensy_firmware.ino.teensy41.hex`

### Step 3: Load with Teensy Loader
1. Open the Teensy Loader application
2. Go to **File > Open HEX File**
3. Navigate to `teensy_firmware/` folder
4. Select `teensy_firmware.ino.teensy41.hex`
5. Press the white button on your Teensy 4.1 board
6. The Teensy Loader will automatically program the board
7. Click **Reboot** if needed

## Building with PlatformIO

### Quick Build (Linux/Mac)
```bash
# Run the build script
./scripts/build_teensy_hex.sh
```

### Quick Build (Windows)
```cmd
REM Run the build script
scripts\build_teensy_hex.bat
```

### Manual Build
```bash
# Clean previous builds
pio run -e teensy41 --target clean

# Build the firmware
pio run -e teensy41

# The HEX file will be in: build_output/teensy41_firmware.hex
```

### Load with Teensy Loader
1. Open Teensy Loader application
2. Go to **File > Open HEX File**
3. Select `build_output/teensy41_firmware.hex`
4. Press the button on your Teensy board
5. Click **Program** in Teensy Loader

## Finding the HEX File

### Arduino IDE
After using "Export Compiled Binary":
- **Location**: `teensy_firmware/teensy_firmware.ino.teensy41.hex`
- The IDE will show the location in the console output

### PlatformIO
After building:
- **Location**: `build_output/teensy41_firmware.hex`
- Or in `.pio/build/teensy41/firmware.hex`

## Teensy Loader Application

### Download
- **Windows/Mac/Linux**: https://www.pjrc.com/teensy/loader.html
- The Teensy Loader is included with Teensyduino installation

### Using Teensy Loader
1. **Open HEX File**: File > Open HEX File, select your .hex file
2. **Put Teensy in Programming Mode**: Press the white button on the Teensy board
3. **Program**: Click the "Program" button (or it will auto-program)
4. **Reboot**: Click "Reboot" to start running the new firmware

### Visual Indicators
- **Green**: HEX file loaded successfully
- **Orange**: Programming in progress  
- **Red**: Error occurred
- Board will automatically reboot after successful programming

## Troubleshooting

### Arduino IDE Issues

**"Teensy board not found"**
- Install Teensyduino: https://www.pjrc.com/teensy/td_download.html
- Restart Arduino IDE after installation

**"FastLED library not found"**
- Go to Sketch > Include Library > Manage Libraries
- Search for "FastLED" and install

**"Export failed"**
- Check that Teensy 4.1 is selected as the board
- Verify USB Type is set to "Serial"
- Try File > Preferences and check "Show verbose output during: compilation"

### PlatformIO Issues

**"Platform 'teensy' not found"**
- Run: `pio platform install teensy`
- Or let PlatformIO auto-install during first build

**"Build failed"**
- Clean build: `pio run -e teensy41 --target clean`
- Try rebuilding: `pio run -e teensy41`

### Teensy Loader Issues

**"HEX file not compatible"**
- Verify you selected Teensy 4.1 as the target board during compilation
- The HEX file must be compiled specifically for Teensy 4.1

**"Unable to program"**
- Press the white button on the Teensy board to enter programming mode
- Try a different USB cable or port
- Update Teensy Loader to latest version

**"Board not detected"**
- Check USB connection
- Try a different USB cable
- Install Teensy USB drivers (Windows)

## Advanced Options

### Optimizing HEX File Size

Edit `platformio.ini` or Arduino IDE settings:
```ini
build_flags = 
    -O2              ; Optimize for speed
    -Os              ; Or optimize for size
    -flto            ; Enable link-time optimization
```

### Custom Build Flags

Add to `teensy_firmware.ino` or `platformio.ini`:
```cpp
// Enable SD card support
#define SD_SUPPORT

// Adjust LED count
#define NUM_LEDS 32
```

## Comparing Arduino IDE vs PlatformIO

| Feature | Arduino IDE | PlatformIO |
|---------|-------------|------------|
| Setup Time | 5 minutes | 10 minutes |
| Ease of Use | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| Build Speed | Medium | Fast |
| Output Location | `teensy_firmware/` | `build_output/` |
| Dependencies | Manual install | Auto-installed |
| Best For | Beginners | Advanced users |

## Next Steps

Once you have the HEX file loaded:
1. Connect to the POV-POI-WiFi network
2. Open http://192.168.4.1 in a browser
3. Control your POV poi via the web interface
4. See the main [README.md](../README.md) for full usage instructions

## Getting Help

If you encounter issues:
- Check the [Troubleshooting Guide](../TROUBLESHOOTING.md)
- Review Teensy documentation: https://www.pjrc.com/teensy/
- PlatformIO docs: https://docs.platformio.org/

---

**Ready to build!** Choose the method that works best for you and create your HEX file for easy deployment with Teensy Loader.
