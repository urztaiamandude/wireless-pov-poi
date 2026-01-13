# Teensy 4.1 Firmware - Arduino IDE Version

This is the **recommended firmware** for the Nebula Poi. It provides a complete, single-file implementation that's easy to understand and modify.

## Features

✅ **Complete Implementation**
- Image display (31x64 pixels max)
- Pattern generation (rainbow, wave, gradient, sparkle)
- Sequence playback with timing and looping
- Live drawing mode
- Brightness control (0-255)
- Frame rate adjustment (10-120 FPS)
- SD card support (optional, v2.0+)

✅ **Easy to Use**
- Single `.ino` file (~530 lines)
- Standard Arduino IDE workflow
- No complex build system
- Quick to upload and test

## Hardware Requirements

- **Teensy 4.1** development board
- **APA102 LED Strip** (32 LEDs: 31 display + 1 level shifter)
- **ESP32** for WiFi communication (via Serial1)
- **5V Power Supply** (2-3A recommended)
- **Optional**: microSD card (for SD_SUPPORT feature)

## Quick Start

### 1. Install Prerequisites
- Arduino IDE 1.8.x or 2.x
- [Teensyduino](https://www.pjrc.com/teensy/td_download.html)
- FastLED library (Install via Library Manager)

### 2. Open Firmware
1. Launch Arduino IDE
2. Open `teensy_firmware.ino`

### 3. Configure Board
- **Board**: Teensy 4.1
- **USB Type**: Serial
- **CPU Speed**: 600 MHz (default)

### 4. Upload
Click the Upload button (→) and wait for completion.

## Configuration Options

### Enable SD Card Support

To enable SD card features, uncomment this line near the top of the file:

```cpp
#define SD_SUPPORT  // Enable SD card storage features
```

**Note**: Requires microSD card in Teensy 4.1's built-in slot.

### Adjust LED Configuration

```cpp
#define NUM_LEDS 32        // Total LEDs (including level shifter)
#define DATA_PIN 11        // APA102 data pin
#define CLOCK_PIN 13       // APA102 clock pin
```

### Serial Communication

```cpp
#define SERIAL_BAUD 115200 // ESP32 communication speed
#define ESP32_SERIAL Serial1 // Hardware serial port
```

## Serial Protocol

The firmware communicates with ESP32 using a binary protocol:

**Format**: `[0xFF][CMD][LEN][DATA...][0xFE]`

### Standard Commands

| Command | Code | Description |
|---------|------|-------------|
| Set Mode | 0x01 | Change display mode (idle/image/pattern/sequence/live) |
| Upload Image | 0x02 | Transfer image data (31x64 max) |
| Upload Pattern | 0x03 | Configure pattern parameters |
| Upload Sequence | 0x04 | Define sequence of items |
| Live Frame | 0x05 | Real-time frame data |
| Set Brightness | 0x06 | Adjust LED brightness (0-255) |
| Set Frame Rate | 0x07 | Adjust display speed (10-120 FPS) |
| Status Request | 0x10 | Query current state |

### SD Card Commands (when SD_SUPPORT enabled)

| Command | Code | Description |
|---------|------|-------------|
| Save to SD | 0x20 | Save image to SD card |
| Load from SD | 0x21 | Load image from SD card |
| List Images | 0x22 | List all stored images |
| Delete Image | 0x23 | Delete image from SD |

## Display Modes

The firmware supports 5 display modes:

0. **Idle** - LEDs off
1. **Image** - Display uploaded POV image
2. **Pattern** - Show animated patterns
3. **Sequence** - Cycle through multiple images/patterns
4. **Live** - Real-time drawing from web interface

## Sequence Playback

Sequences allow cycling through multiple images or patterns with configurable durations:

```cpp
struct Sequence {
  uint8_t items[10];        // Image/pattern indices
  uint16_t durations[10];   // Duration in milliseconds
  uint8_t count;            // Number of items
  bool active;              // Sequence is defined
  bool loop;                // Loop continuously
};
```

The `displaySequence()` function handles:
- Timing each item's display duration
- Advancing to the next item automatically
- Looping back to start (if enabled)
- Stopping at end (if loop disabled)

## Development

### Serial Debugging

Connect to USB Serial at 115200 baud to see debug output:

```
Teensy 4.1 Nebula Poi Initializing...
Teensy 4.1 Nebula Poi Ready!
Commands: IMAGE, PATTERN, SEQUENCE, LIVE, STATUS
```

### Adding Custom Patterns

Add new pattern types in the `displayPattern()` function:

```cpp
void displayPattern() {
  Pattern& pat = patterns[currentIndex];
  
  switch (pat.type) {
    case 0:  // Rainbow
      // Your rainbow code
      break;
    case 5:  // Your custom pattern
      // Your custom pattern code
      break;
  }
}
```

## Troubleshooting

### LEDs Don't Light
- Verify power connections (5V, GND)
- Check DATA_PIN and CLOCK_PIN connections
- Test with lower brightness first

### No Serial Communication
- Verify ESP32 RX/TX connections
- Check baud rate (115200)
- Ensure common ground

### SD Card Issues
- Verify SD_SUPPORT is defined
- Check SD card is formatted (FAT32)
- Ensure card is inserted properly
- Try a different card (Class 10+ recommended)

### Patterns Don't Change
- Check ESP32 communication
- Monitor serial output for errors
- Verify mode is set correctly

## File Structure

```
teensy_firmware.ino
├── Setup & Initialization
│   ├── LED initialization (FastLED)
│   ├── Serial communication setup
│   ├── SD card initialization (if enabled)
│   └── Startup animation
├── Main Loop
│   ├── Serial command processing
│   └── Display updates
├── Data Structures
│   ├── POVImage - Image storage
│   ├── Pattern - Pattern parameters
│   └── Sequence - Sequence definitions
├── Command Handlers
│   ├── receiveImage()
│   ├── receivePattern()
│   ├── receiveSequence()
│   └── receiveLiveFrame()
├── Display Functions
│   ├── displayImage()
│   ├── displayPattern()
│   ├── displaySequence()
│   └── displayLive()
└── SD Card Functions (optional)
    ├── saveImageToSD()
    ├── loadImageFromSD()
    ├── listSDImages()
    └── deleteSDImage()
```

## Version History

- **v2.0** - Added sequence playback and SD card support
- **v1.0** - Initial release with images, patterns, and live mode

## Comparison with PlatformIO Version

| Feature | Arduino IDE (this) | PlatformIO |
|---------|-------------------|------------|
| Setup Complexity | ⭐ Easy | ⭐⭐⭐ Advanced |
| Code Organization | Single file | Modular |
| Feature Complete | ✅ Yes | ⚠️ Partial |
| SD Card Support | ✅ Optional | ✅ Built-in |
| Best For | Quick start, learning | Large projects, teams |

See [FIRMWARE_ARCHITECTURE.md](../FIRMWARE_ARCHITECTURE.md) for detailed comparison.

## Alternative Firmware

If you need a more modular architecture, see `firmware/teensy41/` for the PlatformIO-based version. Note that it's currently under development and may not have all features implemented.

## Support

- **Documentation**: See `/docs` folder
- **Issues**: GitHub Issues
- **Quick Start**: [QUICKSTART.md](../QUICKSTART.md)
- **Troubleshooting**: [TROUBLESHOOTING.md](../TROUBLESHOOTING.md)

## License

See main repository [LICENSE](../LICENSE) file.

---

**Ready to get started?** Follow the [Quick Start Guide](../QUICKSTART.md)!
