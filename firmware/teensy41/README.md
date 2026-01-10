# Teensy 4.1 Firmware

This directory contains the firmware for the Teensy 4.1 microcontroller that drives the POV poi system.

## Overview

The firmware manages:
- **APA102 LED Control**: High-speed LED strip driving for smooth POV display
- **ESP32 Communication**: Serial interface for receiving image data and commands
- **POV Engine**: Persistence of vision display rendering based on rotation

## Hardware Requirements

- Teensy 4.1 development board
- APA102 LED strip (32 LEDs: 31 for display, 1 for level shifting)
- ESP32 co-processor (connected via Serial1)
- Power supply suitable for LED strip

## Pin Configuration

| Function | Pin | Description |
|----------|-----|-------------|
| LED Data | 11 | APA102 data line |
| LED Clock | 13 | APA102 clock line |
| ESP32 RX | 0 | Serial1 RX for ESP32 communication |
| ESP32 TX | 1 | Serial1 TX for ESP32 communication |

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
│   └── pov_engine.h       # POV rendering engine
├── src/
│   ├── main.cpp           # Main application entry point
│   ├── led_driver.cpp     # LED driver implementation
│   ├── esp32_interface.cpp # ESP32 communication implementation
│   └── pov_engine.cpp     # POV engine implementation
├── lib/                   # Project-specific libraries
└── platformio.ini         # PlatformIO configuration
```

## Configuration

Edit `include/config.h` to customize:
- LED strip length (`NUM_LEDS`)
- Pin assignments
- Communication settings
- Display parameters

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

### ESP32 Interface
- Reliable serial communication
- Checksum verification
- Command processing
- Acknowledgment protocol

## Debug Mode

Debug output is available via USB Serial at 115200 baud. Enable by setting `DEBUG_ENABLED` to `true` in `config.h`.

## License

See main repository LICENSE file.
