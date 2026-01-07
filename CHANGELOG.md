# Changelog

All notable changes to the Wireless POV POI project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-01-07

### Added

#### Core Firmware
- Teensy 4.1 firmware with APA102 LED control (32 LEDs)
- ESP32 firmware with WiFi Access Point and web server
- Serial communication protocol between Teensy and ESP32
- FastLED integration for high-performance LED control
- POV image display engine with column-based rendering
- Pattern system with 4 built-in patterns (rainbow, wave, gradient, sparkle)
- Live mode for real-time LED control
- Sequence support for chaining images and patterns
- Adjustable brightness (0-255) and frame rate (10-120 FPS)

#### Web Interface
- Modern, responsive web portal design
- Display mode selection (Idle, Image, Pattern, Sequence, Live)
- System controls with real-time sliders
- Quick pattern buttons with visual feedback
- Color picker for custom pattern colors
- Image upload functionality
- Live drawing canvas with touch support
- Real-time status monitoring
- Mobile-friendly responsive design

#### REST API
- `/api/status` - Get system status
- `/api/mode` - Set display mode
- `/api/brightness` - Adjust brightness
- `/api/framerate` - Set frame rate
- `/api/pattern` - Upload pattern configuration
- `/api/image` - Upload images
- `/api/live` - Send live frame data
- Full JSON request/response format
- Error handling and validation

#### Documentation
- Comprehensive README with quick start guide
- Detailed setup and installation guide (docs/README.md)
- Complete wiring diagram and assembly guide (docs/WIRING.md)
- Full API documentation with examples (docs/API.md)
- Quick start guide (QUICKSTART.md)
- Troubleshooting guide (TROUBLESHOOTING.md)
- Contributing guidelines (CONTRIBUTING.md)
- MIT License

#### Examples and Tools
- Python image converter script
- Android app example with complete source code
- Pattern configuration examples
- Image creation guidelines
- Mobile app integration examples (Kotlin/Swift)

#### Development Tools
- PlatformIO configuration
- Setup verification script
- .gitignore for clean repository
- Structured project organization

### Features

#### Hardware Support
- Teensy 4.1 @ 600 MHz
- ESP32 (any variant with Serial2)
- APA102 LED strips (32 LEDs)
- 5V power supply support
- Serial communication at 115200 baud

#### Display Modes
- **Idle Mode** - LEDs off, low power
- **Image Mode** - Display uploaded POV images
- **Pattern Mode** - Animated pattern display
- **Sequence Mode** - Playback multiple items
- **Live Mode** - Real-time control from web/app

#### Patterns
- **Rainbow** - Rotating rainbow effect across LEDs
- **Wave** - Animated color wave with configurable colors
- **Gradient** - Smooth color gradient between two colors
- **Sparkle** - Random sparkle effect
- Customizable colors and animation speed

#### Network Features
- WiFi Access Point mode (no router needed)
- Default SSID: POV-POI-WiFi
- Default password: povpoi123
- mDNS support (http://povpoi.local)
- Web server on port 80
- SPIFFS file system support

#### Performance
- Up to 120 FPS frame rate
- Sub-50ms command response time
- Support for 31x64 pixel images
- Real-time brightness adjustment
- Efficient serial protocol

### Technical Specifications

- Display Resolution: 31 × 64 pixels (max)
- LED Type: APA102 (BGR color order)
- Serial Protocol: Custom binary protocol
- WiFi: 2.4 GHz Access Point
- Power: 5V, 2-3A recommended
- Operating Temperature: 0-40°C
- WiFi Range: ~30 meters typical

### Known Limitations

- Image processing on ESP32 is simplified
- Maximum 10 images, 5 patterns, 5 sequences
- SPIFFS storage limited by ESP32 partition
- Serial baudrate limits data transfer speed
- No authentication on web interface
- Single client web server

### Dependencies

#### Teensy 4.1
- FastLED library 3.5.0+
- Arduino/Teensyduino

#### ESP32
- ESP32 Arduino Core
- WiFi library (included)
- WebServer library (included)
- ESPmDNS library (included)
- SPIFFS library (included)

### Development Environment

- Arduino IDE 1.8.x or 2.x
- Teensyduino addon
- ESP32 board support
- PlatformIO (optional)

## Future Roadmap

### Planned for v1.1.0
- Image processing improvements
- More pattern types
- Sequence editor in web interface
- Battery monitoring
- OTA firmware updates

### Planned for v1.2.0
- IMU/gyroscope integration
- Rotation-based triggering
- Music synchronization
- Multiple poi synchronization

### Planned for v2.0.0
- Bluetooth LE support
- Native mobile apps (Android/iOS)
- Cloud storage integration
- Advanced pattern editor
- Video playback support

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on contributing to this project.

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

- PJRC for Teensy 4.1
- Espressif for ESP32
- Daniel Garcia for FastLED library
- Community contributors and testers

---

[1.0.0]: https://github.com/urztaiamandude/wireless-pov-poi/releases/tag/v1.0.0
