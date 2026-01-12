# Teensy 4.1 Firmware

This directory contains the firmware for the Teensy 4.1 microcontroller that drives the POV poi system.

## Overview

The firmware manages:
- **APA102 LED Control**: High-speed LED strip driving for smooth POV display
- **ESP32 Communication**: Serial interface for receiving image data and commands
- **POV Engine**: Persistence of vision display rendering based on rotation
- **SD Card Storage**: High-speed SDIO storage for POV images (NEW)

## Hardware Requirements

- Teensy 4.1 development board
- APA102 LED strip (32 LEDs: 31 for display, 1 for level shifting)
- ESP32 co-processor (connected via Serial1)
- microSD card (Class 10 or higher, 16GB-64GB recommended)
- Power supply suitable for LED strip

## Pin Configuration

| Function | Pin | Description |
|----------|-----|-------------|
| LED Data | 11 | APA102 data line |
| LED Clock | 13 | APA102 clock line |
| ESP32 RX | 0 | Serial1 RX for ESP32 communication |
| ESP32 TX | 1 | Serial1 TX for ESP32 communication |
| SD Card | Built-in | Teensy 4.1 built-in microSD slot (SDIO) |

## Building and Uploading

### Prerequisites

- [PlatformIO](https://platformio.org/) installed
- Teensy 4.1 board connected via USB

### Build Commands

```bash
# Navigate to firmware directory
cd firmware/teensy41

# Build the firmware
pio run

# Upload to Teensy 4.1
pio run --target upload

# Open serial monitor
pio device monitor
```

## Project Structure

```
firmware/teensy41/
├── include/
│   ├── config.h           # Hardware and system configuration
│   ├── led_driver.h       # APA102 LED driver interface
│   ├── esp32_interface.h  # ESP32 communication protocol
│   ├── pov_engine.h       # POV rendering engine
│   └── sd_storage.h       # SD card storage manager (NEW)
├── src/
│   ├── main.cpp           # Main application entry point
│   ├── led_driver.cpp     # LED driver implementation
│   ├── esp32_interface.cpp # ESP32 communication implementation
│   ├── pov_engine.cpp     # POV engine implementation
│   └── sd_storage.cpp     # SD card storage implementation (NEW)
├── lib/                   # Project-specific libraries
└── platformio.ini         # PlatformIO configuration
```

## Configuration

Edit `include/config.h` to customize:
- LED strip length (`NUM_LEDS`)
- Pin assignments
- Communication settings
- Display parameters
- SD card settings (`SD_CARD_ENABLED`, `SD_IMAGE_DIR`, etc.)

## Communication Protocol

The firmware uses a simple packet-based protocol for ESP32 communication:

**Message Format:**
```
[TYPE][LENGTH_H][LENGTH_L][DATA...][CHECKSUM]
```

**Message Types:**
- `0x01`: Image data
- `0x02`: Command
- `0x03`: Status
- `0x04`: ACK
- `0x05`: NACK
- `0x10`: Save image to SD (NEW)
- `0x11`: List images on SD (NEW)
- `0x12`: Delete image from SD (NEW)
- `0x13`: Get SD card info (NEW)
- `0x14`: Load image from SD (NEW)

**Commands:**
- `0x01`: Play
- `0x02`: Pause
- `0x03`: Stop
- `0x04`: Set brightness
- `0x05`: Set mode

## Features

### LED Driver
- FastLED library integration for APA102 LEDs
- Brightness control
- High-speed SPI communication
- Individual pixel control

### POV Engine
- Image buffer management
- Column-based rendering
- Rotation tracking
- Multiple display modes
- SD card image loading (NEW)

### SD Card Storage (NEW)
- High-speed SDIO interface (~20-25 MB/s)
- Custom POV image file format
- Save, load, delete, and list operations
- Storage space information
- Graceful error handling
- See [docs/SD_CARD_STORAGE.md](../../docs/SD_CARD_STORAGE.md) for details

### ESP32 Interface
- Reliable serial communication
- Checksum verification
- Command processing
- SD card message handling (NEW)
- Acknowledgment protocol

## Debug Mode

Debug output is available via USB Serial at 115200 baud. Enable by setting `DEBUG_ENABLED` to `true` in `config.h`.

## License

See main repository LICENSE file.
