# Build Overview

This page summarizes the build targets in the Nebula Poi repository and
points to detailed wiki pages for each target.

## Build targets

- Teensy 4.1 firmware (Arduino IDE or PlatformIO)
- ESP32 firmware (Arduino IDE or PlatformIO)
- ESP32-S3 firmware (Arduino IDE or PlatformIO)
- Image converter tools (Python GUI and Windows executable)

## Quick links

- [Teensy firmware build](Build-Teensy-Firmware.md)
- [Teensy HEX files](Build-Teensy-HEX.md)
- [Teensy CLI compilation](Build-Teensy-CLI.md)
- [ESP32 firmware build](Build-ESP32-Firmware.md)
- [Image converter builds](Build-Image-Converter.md)

## PlatformIO quick commands

Use the root `platformio.ini` for unified builds:

```bash
# Build
pio run -e teensy41
pio run -e esp32
pio run -e esp32s3

# Upload
pio run -e teensy41 -t upload
pio run -e esp32 -t upload
pio run -e esp32s3 -t upload
```

## Common output locations

- Teensy HEX (Arduino IDE export):
  `teensy_firmware/teensy_firmware.ino.teensy41.hex`
- Teensy HEX (PlatformIO):
  `.pio/build/teensy41/firmware.hex` and `build_output/teensy41_firmware.hex`
- ESP32 build output (PlatformIO): `.pio/build/esp32/`
- ESP32-S3 build output (PlatformIO): `.pio/build/esp32s3/`
- Windows image converter EXE: `examples/dist/POV_POI_Image_Converter.exe`

## Tooling prerequisites

- Arduino IDE 1.8.x or 2.x and Teensyduino
- PlatformIO (via `pip install platformio`)
- Arduino CLI (optional, for CLI builds)
- Python 3.7+ for image converter tools
