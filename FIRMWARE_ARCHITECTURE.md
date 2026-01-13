# Firmware Architecture Guide

## Overview

This project provides **two firmware implementations** for the Teensy 4.1 microcontroller, each designed for different use cases and user expertise levels.

## 🎯 Quick Decision Guide

**Choose Arduino IDE firmware if you:**
- Want the simplest setup experience
- Are new to embedded development
- Prefer a single-file approach
- Want to quickly get started (30 minutes)
- Don't need advanced modularity

**Choose PlatformIO firmware if you:**
- Are comfortable with professional development tools
- Want a modular, maintainable codebase
- Need SD card storage features
- Plan to extend or customize the firmware significantly
- Prefer structured project organization

## 📂 Firmware Options

### Option 1: Arduino IDE Firmware (Recommended for Beginners)

**Location:** `teensy_firmware/teensy_firmware.ino`

**Characteristics:**
- ✅ Single file implementation (~530 lines)
- ✅ Simple Arduino IDE workflow
- ✅ Complete POV functionality
- ✅ All core features implemented
- ✅ Easy to understand and modify
- ✅ Fast setup (< 5 minutes)
- ✅ Sequence playback (v2.0+)
- ✅ SD card support (v2.0+)
- ❌ Less modular structure

**Features:**
- Image display (31x64 max)
- Pattern generation (rainbow, wave, gradient, sparkle)
- Sequence playback with timing and looping
- Live drawing mode
- Brightness control (0-255)
- Frame rate adjustment (10-120 FPS)
- SD card image storage (optional, compile-time flag)

**Setup:**
1. Install Arduino IDE + Teensyduino
2. Open `teensy_firmware/teensy_firmware.ino`
3. Select Board: Teensy 4.1
4. Select USB Type: Serial
5. Upload

**SD Card Support:**
Enable by uncommenting in the firmware:
```cpp
#define SD_SUPPORT  // Enable SD card features
```

### Option 2: PlatformIO Firmware (Advanced Users)

**Location:** `firmware/teensy41/`

**Characteristics:**
- ✅ Modular architecture (header + implementation files)
- ✅ Professional build system
- ✅ Advanced SD card integration
- ✅ Better code organization
- ✅ Easier to extend and maintain
- ⚠️ More complex setup
- ⚠️ Command processing partially implemented
- ⚠️ Active development

**Structure:**
```
firmware/teensy41/
├── include/
│   ├── config.h           # Configuration constants
│   ├── led_driver.h       # LED control interface
│   ├── esp32_interface.h  # Serial communication
│   ├── pov_engine.h       # POV rendering
│   └── sd_storage.h       # SD card management
├── src/
│   ├── main.cpp           # Entry point
│   ├── led_driver.cpp     # LED implementation
│   ├── esp32_interface.cpp # Communication protocol
│   ├── pov_engine.cpp     # Rendering engine
│   └── sd_storage.cpp     # SD operations
└── platformio.ini         # Build configuration
```

**Features:**
- All Arduino IDE features (when complete)
- High-speed SDIO SD card access (~20-25 MB/s)
- Custom POV image file format
- Advanced error handling
- Modular design for extensions

**Setup:**
1. Install PlatformIO
2. Navigate to `firmware/teensy41/`
3. Run `pio run --target upload`

**Current Status:**
**Current Status:**
- ✅ SD card storage fully implemented
- ✅ LED driver complete
- ✅ POV engine functional
- ✅ ESP32 command processing complete
- ✅ Pattern generation implemented (4 types)
- ✅ Frame rate control implemented
- ✅ Mode management improved
- ⚠️ Integration testing needs hardware validation
- ⚠️ Live drawing needs testing

## 🔄 Feature Parity Comparison

| Feature | Arduino IDE | PlatformIO | Notes |
|---------|-------------|------------|-------|
| Image Display | ✅ | ✅ | 31x64 max |
| Pattern Generation | ✅ | ✅ | 4 patterns (rainbow, wave, gradient, sparkle) |
| Sequence Playback | ✅ | ⚠️ | PIO framework in place |
| Live Drawing | ✅ | ✅ | Implemented, needs testing |
| Brightness Control | ✅ | ✅ | 0-255 range |
| Frame Rate Control | ✅ | ✅ | 10-120 FPS, dynamic adjustment |
| SD Card Support | ✅ (v2.0+) | ✅ | PIO uses SDIO |
| Serial Protocol | ✅ | ✅ | Both simple and structured protocols |
| Web Interface Compatible | ✅ | ✅ | Command processing complete |

Legend: ✅ Complete | ⚠️ Partial/In Development | ❌ Not Available

## 🔌 Protocol Compatibility

Both firmware versions are designed to use the **same serial protocol** for ESP32 communication, ensuring that the web interface works with either firmware once fully implemented.

**Serial Protocol (115200 baud):**
```
Format: [0xFF][CMD][LEN][DATA...][0xFE]
```

**Commands (Both Versions):**
- `0x01` - Set display mode
- `0x02` - Upload image
- `0x03` - Upload pattern
- `0x04` - Upload sequence
- `0x05` - Live frame data
- `0x06` - Set brightness
- `0x07` - Set frame rate
- `0x10` - Status request

**SD Commands (v2.0+ / PlatformIO):**
- `0x20` - Save image to SD
- `0x21` - Load image from SD
- `0x22` - List SD images
- `0x23` - Delete SD image

## 🚀 Migration Path

### From Arduino IDE → PlatformIO

If you want to upgrade to the modular firmware:

1. **Test your current setup** - Ensure everything works
2. **Install PlatformIO** - Follow setup guide
3. **Build PIO firmware** - `pio run -e teensy41`
4. **Upload carefully** - `pio run -e teensy41 --target upload`
5. **Test incrementally** - Verify each feature works

**Note:** Current PIO firmware may not have all features yet. Check status above.

### From PlatformIO → Arduino IDE

If you need a simpler, working solution:

1. **Open Arduino IDE** with Teensyduino installed
2. **Load** `teensy_firmware/teensy_firmware.ino`
3. **Select Board** - Teensy 4.1, USB Type: Serial
4. **Upload** - Standard Arduino IDE upload process

## 🛠️ Development Workflow

### Arduino IDE Firmware

**Typical workflow:**
1. Modify `teensy_firmware.ino`
2. Upload via Arduino IDE
3. Monitor Serial output (115200 baud)
4. Test with web interface

**Best for:**
- Quick experiments
- Learning POV programming
- Simple customizations
- Rapid prototyping

### PlatformIO Firmware

**Typical workflow:**
1. Modify relevant `.cpp`/`.h` files
2. Build: `pio run -e teensy41`
3. Upload: `pio run --target upload`
4. Monitor: `pio device monitor`

**Best for:**
- Large-scale modifications
- Adding new modules
- Professional development
- Team collaboration
- Version control friendly

## 📚 Documentation References

### For Arduino IDE Firmware:
- [Quick Start Guide](QUICKSTART.md) - Step-by-step setup
- [API Documentation](docs/API.md) - Serial protocol details
- [Wiring Guide](docs/WIRING.md) - Hardware connections

### For PlatformIO Firmware:
- [PlatformIO README](firmware/teensy41/README.md) - Detailed setup
- [SD Card Storage](docs/SD_CARD_STORAGE.md) - SD features (if exists)
- [Build Configuration](platformio.ini) - Build settings

## ❓ FAQ

### Which firmware should I use for production?

**Currently:** Arduino IDE firmware (stable, complete features)
**Future:** PlatformIO firmware (when command processing is complete)

### Can I switch between firmwares?

Yes! Both use the same hardware connections. Simply upload the firmware you want to use.

### Will my web interface work with both?

Yes, when PlatformIO firmware is complete. Currently, Arduino IDE firmware is fully compatible.

### How do I enable SD card support?

**Arduino IDE:** Uncomment `#define SD_SUPPORT` in `teensy_firmware.ino`
**PlatformIO:** Enabled by default, configured in `config.h`

### What if I have issues?

1. Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
2. Verify hardware connections
3. Check Serial Monitor output (115200 baud)
4. Try the other firmware version
5. Open an issue on GitHub

## 🔮 Future Direction

The project is moving toward:
- **Unified codebase** - Merge best features of both
- **Complete PIO implementation** - Full feature parity
- **Automated testing** - Unit tests for both versions
- **Improved documentation** - Video tutorials
- **Enhanced SD features** - Playlists, metadata

## 🤝 Contributing

Contributions to either firmware version are welcome!

**Arduino IDE firmware priorities:**
- Performance optimizations
- New pattern types
- Enhanced sequence features

**PlatformIO firmware priorities:**
- Complete command processing
- Integration testing
- Feature parity with Arduino version

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

**Need Help?** Check the [Quick Start Guide](QUICKSTART.md) or open an issue!
