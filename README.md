# Nebula Poi

A wireless persistence of vision (POV) LED poi system featuring Teensy 4.1 and ESP32/ESP32-S3 with web-based control interface.

## Overview

This system creates stunning POV light displays using a 32 LED APA102 strip controlled by a Teensy 4.1 microcontroller. An ESP32 or ESP32-S3 co-processor provides WiFi connectivity, enabling wireless control through a built-in web portal accessible from any device (phone, tablet, laptop). The system supports custom images, animated patterns, sequences, and real-time live drawing mode.

## Features

### Hardware
- **Teensy 4.1** - High-performance main controller for LED display and POV rendering
- **ESP32 or ESP32-S3** - WiFi co-processor for wireless communication and web interface
  - ESP32-S3 N16R8 (16MB Flash, 8MB PSRAM) recommended for new builds ✨
  - 📋 [ESP32-S3 Purchase Guide](ESP32_S3_PURCHASE_GUIDE.md) - Should I buy ESP32-S3?
  - 🔧 [ESP32-S3 Compatibility Guide](docs/ESP32_S3_COMPATIBILITY.md) - Technical details
- **APA102 LED Strip** - 32 addressable RGB LEDs (31 for display, 1 for level shifting)
- **MAX9814 Microphone** (optional) - For music-reactive pattern modes
- Serial communication (115200 baud) between Teensy and ESP32/ESP32-S3

### Display Capabilities
- **POV Image Display** - Upload and display custom images with persistence of vision
- **16 Animated Patterns** - Complete set of built-in visual effects:
  - **Basic Patterns (0-10):** Rainbow, Wave, Gradient, Sparkle, Fire, Comet, Breathing, Strobe, Meteor, Wipe, Plasma
  - **Music-Reactive Patterns (11-15):** VU Meter, Pulse, Audio Rainbow, Center Burst, Audio Sparkle (requires microphone)
- **Sequences** - Chain multiple images and patterns together
- **Live Mode** - Real-time drawing and control from web interface
- **Adjustable Settings** - Control brightness (0-255) and frame rate (10-120 FPS)

### Peer-to-Peer Synchronization 🆕
- **Multi-Device Pairing** - Sync multiple poi devices together
- **Automatic Discovery** - Devices find each other via mDNS
- **Bidirectional Sync** - Share images, patterns, and settings between devices
- **Independent Operation** - Each device works standalone, sync when convenient
- **No Master/Slave** - All devices are equal peers
- **See [POI Pairing Guide](docs/POI_PAIRING.md)** for setup instructions

### Wireless Control
- **WiFi Access Point** - Direct wireless connection without router
  - SSID: `POV-POI-WiFi`
  - Password: `povpoi123`
  - IP: `192.168.4.1`
- **Bluetooth Low Energy (BLE)** - Direct BLE connectivity for clients
  - Device Name: `Wireless POV Poi`
  - Nordic UART Service (NUS) for cross-platform compatibility
  - Works with Windows and Web (Chrome/Edge)
  - **Operates completely offline** - no internet required
  - See [BLE Protocol Documentation](docs/BLE_PROTOCOL.md)
- **Web Portal** - Full-featured control interface accessible via browser
- **REST API** - Complete API for web UI and integrations
- **mDNS Support** - Access via `http://povpoi.local`

## Firmware Architecture

This project offers **two firmware implementations** for Teensy 4.1:

1. **Arduino IDE Firmware** (`teensy_firmware/`) - **Recommended for most users**
   - Single-file, easy to understand
   - Complete features including sequences and SD card support
   - Quick setup with Arduino IDE
   - Best for beginners and quick deployment

2. **PlatformIO Firmware** (`firmware/teensy41/`) - **For advanced users**
   - Modular architecture with separate modules
   - Professional build system
   - Advanced SD card integration
   - Better for large-scale customization
   - ⚠️ Command processing partially implemented

**👉 See [FIRMWARE_ARCHITECTURE.md](FIRMWARE_ARCHITECTURE.md) for detailed comparison and selection guide**

## Quick Start

### 1. Hardware Setup
See [docs/WIRING.md](docs/WIRING.md) for detailed wiring instructions.

**Basic Connections** (works for all ESP32 variants: WROOM-32, DevKitC, S3):
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
1. Open `teensy_firmware/teensy_firmware.ino` (recommended) or use PlatformIO version
2. Select Board: Teensy 4.1
3. Select USB Type: Serial
4. Upload the firmware

**Alternative: Build HEX file for Teensy Loader**
- Use Sketch > Export Compiled Binary in Arduino IDE
- Or use CLI build scripts: `./scripts/build_arduino_cli.sh` or `./scripts/build_teensy_hex.sh` (Linux/Mac)
- **Build Wiki**: See [Build Overview](wiki/Build-Overview.md),
  [Teensy HEX](wiki/Build-Teensy-HEX.md), and
  [Teensy CLI](wiki/Build-Teensy-CLI.md)
- **Quick Guide**: See [QUICK_HEX_GUIDE.md](QUICK_HEX_GUIDE.md) for fast start

**Program the ESP32 or ESP32-S3:**

**Option A: Using Arduino IDE**
1. Open `esp32_firmware/esp32_firmware.ino`
2. Select Board: 
   - For ESP32: ESP32 Dev Module
   - For ESP32-S3: ESP32S3 Dev Module
3. Upload the firmware

**Option B: Using PlatformIO**
```bash
# For ESP32
pio run -e esp32 -t upload

# For ESP32-S3 (recommended for new builds)
pio run -e esp32s3 -t upload
```

See [ESP32-S3 Compatibility Guide](docs/ESP32_S3_COMPATIBILITY.md) for ESP32-S3 setup details.

### 3. Connect and Control

**Option A: WiFi Connection**
1. Power on the system
2. Connect to WiFi network: **POV-POI-WiFi** (password: `povpoi123`)
3. Open browser and navigate to: `http://192.168.4.1`
4. Use the web interface to control patterns, upload images, and adjust settings

**Option B: Bluetooth Low Energy (BLE) Connection**
1. Power on the system
2. Open a BLE-capable app (Flutter app, nRF Connect, etc.)
3. Scan for BLE devices and look for: **Wireless POV Poi**
4. Connect to the device
5. Use the Nordic UART Service to send commands
6. See [BLE Protocol Documentation](docs/BLE_PROTOCOL.md) for command reference

**Note:** BLE and WiFi can operate simultaneously. BLE is more power-efficient and has lower latency for simple commands.

**🎨 Demo Content Available!** The firmware comes pre-loaded with:
- 3 demo images (Smiley, Rainbow, Heart)
- 5 demo patterns (Rainbow, Fire, Comet, Breathing, Plasma)
- 1 demo sequence (cycles through images and patterns)

See [DEMO_CONTENT.md](DEMO_CONTENT.md) for complete details on built-in content.

## Project Status

🎉 **This project is production-ready!** All documented features are complete and fully functional.

- **[Project Status](PROJECT_STATUS.md)** - Quick status overview and what works now
- **[Remaining Work Analysis](REMAINING_WORK.md)** - Detailed analysis of incomplete items (PlatformIO firmware only)

## Documentation

- **[Complete Setup Guide](docs/README.md)** - Detailed installation and usage instructions
- **[Wiring Diagram](docs/WIRING.md)** - Hardware connections and assembly guide
- **[API Documentation](docs/API.md)** - REST API reference for web UI and integrations
- **[BLE Protocol](docs/BLE_PROTOCOL.md)** - Bluetooth Low Energy command reference
- **[POI Pairing Guide](docs/POI_PAIRING.md)** 🆕 - Setup and sync multiple poi devices
- **[BMP Image Processing Guide](docs/BMP_IMAGE_PROCESSING.md)** 🆕 - BMPImageReader and BMPImageSequence usage
- **[Image Conversion Guide](docs/IMAGE_CONVERSION.md)** - How automatic image conversion works
- **[Testing Guide](TESTING.md)** - Testing tools, environment setup, and test procedures

## Web Interface

The web portal provides a modern, **mobile-responsive** interface with:
- Display mode selection (Idle, Image, Pattern, Sequence, Live)
- System controls (brightness and frame rate sliders)
- Quick pattern buttons (Rainbow, Wave, Gradient, Sparkle)
- Color picker for custom pattern colors
- **Image upload with automatic conversion** (any size → 31px HIGH, width calculated from aspect ratio)
- Live drawing canvas for real-time control
- Real-time status display
- **PWA support** - Install as native app on mobile devices
- **Touch-optimized controls** for mobile use

### Image Orientation

The LED strip forms the VERTICAL axis of the POV display:
- **HEIGHT is FIXED at 31 pixels** - One pixel per display LED
- **WIDTH is CALCULATED** - Based on original image aspect ratio
- **LED 1** (bottom of strip) displays the **bottom** of the image
- **LED 31** (top of strip) displays the **top** of the image
- **No flip needed** - The LED arrangement maps directly to image pixels
- When poi are spun, images scroll naturally in correct orientation

### REST API

The system includes a complete REST API for the web UI and integrations:
- Status monitoring
- Mode control
- Brightness adjustment
- Pattern configuration
- Image upload with automatic conversion
- Live frame updates

See [API Documentation](docs/API.md) for detailed endpoint information and example code.

## Technical Specifications

- **LED Controller**: Teensy 4.1 @ 600 MHz
- **WiFi Module**: ESP32 or ESP32-S3 (2.4 GHz)
  - ESP32: 4MB Flash typical
  - ESP32-S3 N16R8: 16MB Flash + 8MB PSRAM (recommended) ✨
- **LED Strip**: APA102 (32 LEDs, 31 for display + 1 for level shifting)
- **Display Resolution**: HEIGHT = 31 pixels (fixed), WIDTH = variable
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

## Image Conversion

Convert images to POV-compatible format (31 pixels HIGH):

### Option 1: GUI Converter (Recommended for Desktop)
```bash
cd examples
pip install -r requirements.txt
python image_converter_gui.py
```

**Features:**
- Visual interface with before/after preview
- Adjust conversion settings in real-time
- Single image or batch conversion support
- Cross-platform (Windows, Mac, Linux)

### Option 2: Command-Line Converter
```bash
cd examples
python image_converter.py your_image.jpg
```

### Option 3: Web Interface (No Installation)
Upload directly through the POV device's web interface at http://192.168.4.1 - images are automatically converted

### Option 4: Online Tool
- **POISonic Online Converter**: [https://web.archive.org/web/20210509110926/https://www.orchardelica.com/poisonic/poi_page.html](https://web.archive.org/web/20210509110926/https://www.orchardelica.com/poisonic/poi_page.html) - Browser-based converter

## BMP Image Processing with BMPImageReader

The firmware now includes `BMPImageReader` and `BMPImageSequence` classes from the pov-library for robust BMP image handling.

### Features

- **BMPImageReader**: Standalone BMP header parsing and line-by-line access
- **BMPImageSequence**: Playlist management with durations
- Works directly with Teensy 4.1's built-in SD card
- Memory efficient with user-controlled buffers
- Template-based design for maximum flexibility

### Usage with SD Card

```cpp
#include "BMPImageReader.h"

BMPImageReader reader;
File bmpFile = SD.open("image.bmp");

if (reader.begin(bmpFile)) {
    Serial.print("Image: ");
    Serial.print(reader.width());
    Serial.print(" x ");
    Serial.println(reader.height());
    
    uint8_t* buffer = new uint8_t[reader.bufferSize()];
    reader.loadToBuffer(bmpFile, buffer);
    
    // Access line-by-line for POV display
    for (int y = 0; y < reader.height(); y++) {
        uint8_t* line = reader.getLine(buffer, y);
        // Display line on LEDs
    }
    
    delete[] buffer;
}
```

### Image Sequences

Create an `imagelist.txt` file on your SD card:
```
image1.bmp 20
image2.bmp 15
image3.bmp 10
```

Load and use sequences:
```cpp
#include "BMPImageSequence.h"

BMPImageSequence sequence;
File listFile = SD.open("imagelist.txt");
sequence.loadFromFile(listFile);
listFile.close();

// In your display loop
const char* filename = sequence.getCurrentFilename();
uint16_t duration = sequence.getCurrentDuration();
// ... display image ...
sequence.next(); // Move to next image
```

### Requirements

- Images must be 24-bit uncompressed BMP format
- Width and height can be any size (limited by available RAM)
- For POV display, height should match your LED count (31 pixels)

See [BMP Image Processing Guide](docs/BMP_IMAGE_PROCESSING.md) for complete documentation and examples.

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
