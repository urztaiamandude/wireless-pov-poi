# wireless-pov-poi

Teensy 4.1-powered POV poi system featuring APA102 LEDs. Integrates an ESP32 co-processor to manage wireless communication and data functionality.

## Project Structure

```
wireless-pov-poi/
├── firmware/
│   └── teensy41/          # Teensy 4.1 firmware
│       ├── src/           # Source files
│       ├── include/       # Header files
│       └── platformio.ini # Build configuration
└── README.md
```

## Getting Started

### Teensy 4.1 Firmware

The Teensy 4.1 handles real-time LED control and POV rendering. See [firmware/teensy41/README.md](firmware/teensy41/README.md) for detailed build and configuration instructions.

**Quick Start:**
```bash
cd firmware/teensy41
pio run --target upload
```

## Hardware

- **Teensy 4.1**: Main microcontroller
- **APA102 LED Strip**: High-speed addressable LEDs (144 LEDs default)
- **ESP32**: Co-processor for wireless communication
- **Power Supply**: Adequate for LED strip power requirements

## Features

- High-speed POV (Persistence of Vision) display
- APA102 LED control via FastLED
- Serial communication with ESP32 co-processor
- Configurable display parameters
- Real-time image rendering
