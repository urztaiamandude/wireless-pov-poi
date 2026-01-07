# Wireless POV POI System

A complete wireless persistence of vision (POV) LED poi system featuring Teensy 4.1 and ESP32 with web-based control interface.

## Overview

This system creates stunning POV light displays using a 32 LED APA102 strip controlled by a Teensy 4.1 microcontroller. An ESP32 co-processor provides WiFi connectivity, enabling wireless control through a built-in web portal accessible from any device (phone, tablet, laptop). The system supports custom images, animated patterns, sequences, and real-time live drawing mode.

## Features

### Hardware
- **Teensy 4.1** - High-performance main controller for LED display and POV rendering
- **ESP32** - WiFi co-processor for wireless communication and web interface
- **APA102 LED Strip** - 32 addressable RGB LEDs (31 for display, 1 for level shifting)
- Serial communication (115200 baud) between Teensy and ESP32

### Display Capabilities
- **POV Image Display** - Upload and display custom images with persistence of vision
- **Animated Patterns** - Built-in patterns including rainbow, wave, gradient, and sparkle
- **Sequences** - Chain multiple images and patterns together
- **Live Mode** - Real-time drawing and control from web interface
- **Adjustable Settings** - Control brightness (0-255) and frame rate (10-120 FPS)

### Wireless Control
- **WiFi Access Point** - Direct wireless connection without router
  - SSID: `POV-POI-WiFi`
  - Password: `povpoi123`
  - IP: `192.168.4.1`
- **Web Portal** - Full-featured control interface accessible via browser
- **REST API** - Complete API for mobile app integration (Android/iOS)
- **mDNS Support** - Access via `http://povpoi.local`

## Quick Start

### 1. Hardware Setup
See [docs/WIRING.md](docs/WIRING.md) for detailed wiring instructions.

**Basic Connections:**
- Teensy Pin 11 → APA102 Data (DI)
- Teensy Pin 13 → APA102 Clock (CI)
- Teensy TX1 (Pin 1) → ESP32 RX2 (GPIO 16)
- Teensy RX1 (Pin 0) → ESP32 TX2 (GPIO 17)
- Common Ground for all components
- 5V power supply (2-3A recommended)

### 2. Software Setup

**Install Required Software:**
- Arduino IDE 1.8.x or 2.x
- [Teensyduino](https://www.pjrc.com/teensy/td_download.html) for Teensy 4.1 support
- ESP32 board support for Arduino
- FastLED library (via Arduino Library Manager)

**Program the Teensy 4.1:**
1. Open `teensy_firmware/teensy_firmware.ino`
2. Select Board: Teensy 4.1
3. Select USB Type: Serial
4. Upload the firmware

**Program the ESP32:**
1. Open `esp32_firmware/esp32_firmware.ino`
2. Select Board: ESP32 Dev Module
3. Upload the firmware

### 3. Connect and Control

1. Power on the system
2. Connect to WiFi network: **POV-POI-WiFi** (password: `povpoi123`)
3. Open browser and navigate to: `http://192.168.4.1`
4. Use the web interface to control patterns, upload images, and adjust settings

## Documentation

- **[Complete Setup Guide](docs/README.md)** - Detailed installation and usage instructions
- **[Wiring Diagram](docs/WIRING.md)** - Hardware connections and assembly guide
- **[API Documentation](docs/API.md)** - REST API reference for mobile app development

## Web Interface

The web portal provides a modern, responsive interface with:
- Display mode selection (Idle, Image, Pattern, Sequence, Live)
- System controls (brightness and frame rate sliders)
- Quick pattern buttons (Rainbow, Wave, Gradient, Sparkle)
- Color picker for custom pattern colors
- Image upload functionality
- Live drawing canvas for real-time control
- Real-time status display

## Mobile App Support

The system includes a complete REST API for building mobile apps:
- Status monitoring
- Mode control
- Brightness adjustment
- Pattern configuration
- Image upload
- Live frame updates

See [API Documentation](docs/API.md) for detailed endpoint information and example code for Android and iOS.

## Technical Specifications

- **LED Controller**: Teensy 4.1 @ 600 MHz
- **WiFi Module**: ESP32 (2.4 GHz)
- **LED Strip**: APA102 (32 LEDs)
- **Display Resolution**: 31 pixels × 64 pixels max
- **Frame Rate**: 10-120 FPS (adjustable)
- **Brightness**: 0-255 (adjustable)
- **Serial Baudrate**: 115200 bps
- **Power Requirements**: 5V, 2-3A
- **WiFi Range**: ~30 meters typical

## POV Display Tips

For best results with persistence of vision:
- Spin at consistent speed (2-3 rotations per second)
- Use in low-light environments
- High contrast images work best
- Simple graphics better than complex photos
- Higher frame rates produce smoother images

## Customization

### Change WiFi Credentials
Edit `esp32_firmware/esp32_firmware.ino`:
```cpp
const char* ssid = "POV-POI-WiFi";
const char* password = "povpoi123";
```

### Add Custom Patterns
Edit `teensy_firmware/teensy_firmware.ino` and add new pattern types in the `displayPattern()` function.

### Adjust LED Configuration
Modify these defines in `teensy_firmware/teensy_firmware.ino`:
```cpp
#define NUM_LEDS 32
#define DATA_PIN 11
#define CLOCK_PIN 13
```

## Troubleshooting

**LEDs not lighting:**
- Check power supply connections
- Verify APA102 data and clock connections
- Test with lower brightness first

**Can't connect to WiFi:**
- Verify ESP32 is powered and programmed
- Check SSID and password
- Try direct IP: `192.168.4.1`

**Patterns not changing:**
- Check serial connection between Teensy and ESP32
- Monitor serial output for debug messages
- Verify both devices are powered

See [docs/README.md](docs/README.md) for more troubleshooting information.

## Safety

- Use appropriate power supply ratings (5V, 2-3A minimum)
- Avoid looking directly at LEDs at full brightness
- Ensure secure mounting of all components
- Be aware of spinning poi safety when in use
- Properly insulate all electrical connections

## Future Enhancements

Potential additions:
- IMU/gyroscope for rotation detection
- Battery power with charge management
- SD card storage for more images
- Bluetooth LE support
- Music synchronization
- Multiple poi synchronization
- OTA firmware updates

## License

Open source project - feel free to modify and distribute.

## Credits

Built with:
- Teensy 4.1 by PJRC
- ESP32 by Espressif Systems
- FastLED library by Daniel Garcia
- APA102 LED technology

## Contributing

Contributions welcome! Feel free to:
- Report issues
- Suggest enhancements
- Submit pull requests
- Share your POV designs

---

**Created for wireless POV poi applications** - Spin on! 🎨✨
