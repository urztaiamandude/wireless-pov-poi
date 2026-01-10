# Wireless POV POI System

A complete persistence of vision (POV) LED poi system using Teensy 4.1 and ESP32 with wireless control via web interface and mobile app support.

## Features

### Hardware
- **Teensy 4.1** - Main controller for LED display and POV rendering
- **APA102 LED Strip** - 32 addressable RGB LEDs (31 for display, 1 for level shifting)
- **ESP32** - WiFi co-processor for wireless communication and web interface
- Serial communication between Teensy and ESP32

### Capabilities
- Display custom images in POV mode
- Multiple built-in patterns (rainbow, wave, gradient, sparkle)
- Sequence support for playing multiple images/patterns
- Live drawing mode for real-time display
- Web-based control interface
- Mobile app API support
- Adjustable brightness and frame rate
- WiFi Access Point for direct connection

## Hardware Setup

### Components Required
- Teensy 4.1 development board
- ESP32 development board (ESP32-DevKitC or similar)
- APA102 LED strip (32 LEDs, 30 LEDs/m or 60 LEDs/m)
- 5V power supply (min 2A recommended)
- Logic level shifter (or use LED 0 as level shifter)
- Wiring and connectors

### Wiring Diagram

#### Teensy 4.1 Connections
```
Teensy 4.1          APA102 LED Strip
Pin 11 (MOSI)   →   DATA (DI)
Pin 13 (SCK)    →   CLOCK (CI)
GND             →   GND
5V              →   5V (from power supply)

Teensy 4.1          ESP32
Pin 0 (RX1)     ←   GPIO 17 (TX2)
Pin 1 (TX1)     →   GPIO 16 (RX2)
GND             →   GND
```

#### ESP32 Connections
```
ESP32               Teensy 4.1
GPIO 16 (RX2)   ←   Pin 1 (TX1)
GPIO 17 (TX2)   →   Pin 0 (RX1)
GND             →   GND

ESP32               Power
5V              →   5V (USB or power supply)
GND             →   GND
```

### Power Considerations
- APA102 LEDs: ~60mA per LED at full brightness
- 32 LEDs max draw: ~2A at full white
- Use adequate power supply (5V, 2-3A recommended)
- Consider separate power for LEDs vs microcontrollers
- Add capacitor (1000µF) across LED power lines

## Software Setup

### Prerequisites
- Arduino IDE 1.8.x or 2.x
- Teensyduino addon (for Teensy 4.1 support)
- ESP32 board support for Arduino

### Installing Board Support

#### Teensy 4.1
1. Download and install Teensyduino from https://www.pjrc.com/teensy/td_download.html
2. Select Teensy 4.1 as target board in Arduino IDE
3. Install FastLED library via Library Manager

#### ESP32
1. Add ESP32 board manager URL to Arduino IDE preferences:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. Install "ESP32 by Espressif Systems" via Board Manager
3. Select appropriate ESP32 board (e.g., "ESP32 Dev Module")

### Required Libraries

#### For Teensy 4.1
- **FastLED** (version 3.5.0 or later)
  - Install via Arduino Library Manager
  - Search for "FastLED" and install

#### For ESP32
- **WiFi** (included with ESP32 core)
- **WebServer** (included with ESP32 core)
- **ESPmDNS** (included with ESP32 core)
- **SPIFFS** (included with ESP32 core)

### Programming the Teensy 4.1

1. Open `teensy_firmware/teensy_firmware.ino` in Arduino IDE
2. Select **Tools > Board > Teensy 4.1**
3. Select **Tools > USB Type > Serial**
4. Select **Tools > CPU Speed > 600 MHz**
5. Click Upload button
6. Press button on Teensy if required

### Programming the ESP32

1. Open `esp32_firmware/esp32_firmware.ino` in Arduino IDE
2. Select **Tools > Board > ESP32 Dev Module** (or your specific board)
3. Select the correct COM port
4. Set **Tools > Partition Scheme > Default** or **Minimal SPIFFS**
5. Click Upload button
6. Hold BOOT button on ESP32 if required during upload

## Usage

### Connecting to the System

1. Power on both Teensy and ESP32
2. ESP32 creates WiFi Access Point: **POV-POI-WiFi**
3. Password: **povpoi123**
4. Connect your device to this network
5. Open browser and navigate to:
   - http://192.168.4.1
   - or http://povpoi.local (if mDNS works)

### Web Interface Guide

#### Display Modes
- **Idle** - LEDs off
- **Image Display** - Show uploaded POV images
- **Pattern Display** - Show animated patterns
- **Sequence** - Play sequence of images/patterns
- **Live Mode** - Real-time drawing/control

#### System Controls
- **Brightness** - Adjust LED brightness (0-255)
- **Frame Rate** - Set refresh rate (10-120 FPS)

#### Quick Patterns
- **Rainbow** - Rotating rainbow effect
- **Wave** - Color wave animation
- **Gradient** - Smooth color gradient
- **Sparkle** - Random sparkle effect

#### Image Upload
1. Click "Select Image" button
2. Choose image file (JPG, PNG, etc.)
3. Click "Upload & Display"
4. System converts and displays image in POV mode

#### Live Draw Mode
- Draw on canvas with mouse or touch
- Colors update in real-time on LEDs
- Use "Clear" to reset canvas

### Mobile App Integration

The system provides a REST API for mobile app development:

#### API Endpoints

**Get Status**
```
GET /api/status
Response: {"connected": true, "mode": 2, "index": 0, "brightness": 128, "framerate": 50}
```

**Set Mode**
```
POST /api/mode
Body: {"mode": 2, "index": 0}
```

**Set Brightness**
```
POST /api/brightness
Body: {"brightness": 200}
```

**Set Frame Rate**
```
POST /api/framerate
Body: {"framerate": 60}
```

**Upload Pattern**
```
POST /api/pattern
Body: {"index": 0, "type": 0, "color1": {...}, "color2": {...}, "speed": 50}
```

**Send Live Frame**
```
POST /api/live
Body: {"pixels": [{r: 255, g: 0, b: 0}, ...]}
```

## POV Display Technique

POV (Persistence of Vision) works by rapidly displaying different columns of an image as the poi spins. Your eye perceives this as a complete image due to persistence of vision.

### Tips for Best Results
- Spin at consistent speed (2-3 rotations per second)
- Use higher frame rates for smoother display
- Images should be 31 pixels wide (matching LED count)
- Height can be up to 64 pixels
- High contrast images work best
- Use in low-light environments for best effect

### Image Conversion Tools

To convert your images to POV-compatible format:
- **Python Script**: Use `examples/image_converter.py` included in this repository
- **POISonic Online Converter**: [https://web.archive.org/web/20210509110926/https://www.orchardelica.com/poisonic/poi_page.html](https://web.archive.org/web/20210509110926/https://www.orchardelica.com/poisonic/poi_page.html) - Browser-based tool for quick conversions

## Customization

### Changing WiFi Credentials

Edit `esp32_firmware/esp32_firmware.ino`:
```cpp
const char* ssid = "POV-POI-WiFi";      // Change SSID
const char* password = "povpoi123";      // Change password
```

### Adding Custom Patterns

In `teensy_firmware/teensy_firmware.ino`, add new pattern types in the `displayPattern()` function:
```cpp
case 4:  // Your custom pattern
  // Add your pattern code here
  break;
```

### Adjusting LED Configuration

Change these values in `teensy_firmware/teensy_firmware.ino`:
```cpp
#define NUM_LEDS 32        // Total LEDs
#define DATA_PIN 11        // Data pin
#define CLOCK_PIN 13       // Clock pin
#define DISPLAY_LEDS 31    // Active display LEDs
```

## Troubleshooting

### LEDs Not Lighting Up
- Check power supply voltage and current capacity
- Verify APA102 wiring (DATA and CLOCK pins)
- Check LED 0 is properly configured for level shifting
- Test with simple FastLED example first

### Cannot Connect to WiFi
- Verify ESP32 is powered and programmed
- Check WiFi credentials match
- Try connecting to IP directly: 192.168.4.1
- Check ESP32 serial monitor for IP address

### Teensy Not Responding
- Verify serial connection between Teensy and ESP32
- Check baud rate matches (115200)
- Check RX/TX pins are correctly connected
- Monitor Teensy Serial output for debug messages

### Patterns Not Changing
- Check mode is set correctly in web interface
- Verify Teensy is receiving commands (check Serial monitor)
- Ensure ESP32-Teensy serial connection is working
- Try power cycling both boards

### Images Not Displaying
- Verify image was uploaded successfully
- Check image is in correct mode (mode 1)
- Ensure image dimensions are compatible
- Try with built-in patterns first

## Performance Notes

- Frame rate affects smoothness and power consumption
- Higher brightness increases power draw significantly
- APA102 supports very high refresh rates (20kHz+)
- Serial communication baudrate is 115200
- Web server can handle multiple concurrent connections
- SPIFFS storage limited (check ESP32 partition scheme)

## Safety Considerations

- Use appropriate power supply ratings
- Avoid looking directly at LEDs at full brightness
- Ensure secure mounting of components
- Check wire connections before powering
- Use proper insulation for exposed connections
- Be aware of spinning poi safety

## Future Enhancements

Possible improvements and additions:
- IMU/gyroscope for rotation sensing
- Battery power with charge management
- SD card for storing more images
- Bluetooth LE support
- Mobile app (Android/iOS)
- Pre-programmed sequences
- Music synchronization
- Multiple poi synchronization
- OTA (Over-The-Air) firmware updates

## License

This project is open source. Feel free to modify and distribute.

## Credits

Created for wireless POV poi applications using:
- Teensy 4.1 by PJRC
- ESP32 by Espressif
- FastLED library by Daniel Garcia
- APA102 LED technology

## Support

For issues, questions, or contributions:
- Check troubleshooting section
- Review serial monitor output
- Verify wiring and connections
- Test components individually

Happy spinning! 🎨✨
