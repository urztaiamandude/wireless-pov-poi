# Build Scripts

This directory contains scripts for building the Teensy 4.1 firmware into HEX format for use with the Teensy Loader application.

## Available Scripts

### `build_teensy_hex.sh` (Linux/Mac)
Bash script to compile Teensy firmware to HEX format using PlatformIO.

**Usage:**
```bash
./scripts/build_teensy_hex.sh
```

**Prerequisites:**
- PlatformIO installed: `pip install platformio`

### `build_teensy_hex.bat` (Windows)
Batch script to compile Teensy firmware to HEX format using PlatformIO.

**Usage:**
```cmd
scripts\build_teensy_hex.bat
```

**Prerequisites:**
- PlatformIO installed: `pip install platformio`

### `post_build_teensy.py`
PlatformIO post-build script that automatically:
- Copies the generated HEX file to `build_output/teensy41_firmware.hex`
- Displays instructions for loading with Teensy Loader

This script is called automatically by PlatformIO during the build process.

## Output

After building, the HEX file will be located at:
```
build_output/teensy41_firmware.hex
```

This file can be loaded directly into the Teensy Loader application.

## Alternative: Arduino IDE Method

The easiest way to generate a HEX file is using Arduino IDE:

1. Open `teensy_firmware/teensy_firmware.ino` in Arduino IDE
2. Select **Tools > Board > Teensy 4.1**
3. Select **Tools > USB Type > Serial**
4. Go to **Sketch > Export Compiled Binary**
5. The HEX file will be created in the `teensy_firmware` directory

See [docs/BUILDING_HEX.md](../docs/BUILDING_HEX.md) for detailed instructions.

## Troubleshooting

If the PlatformIO build scripts fail:
- Ensure PlatformIO is installed: `pio --version`
- Try cleaning first: `pio run -e teensy41 --target clean`
- Check internet connection (PlatformIO needs to download platform packages on first run)

For the Arduino IDE method, ensure:
- Teensyduino is installed: https://www.pjrc.com/teensy/td_download.html
- FastLED library is installed via Library Manager

## More Information

For complete documentation on building and loading HEX files, see:
- [Building HEX Files Guide](../docs/BUILDING_HEX.md)
- [Main README](../README.md)
