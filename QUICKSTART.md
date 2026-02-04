# Quick Start Guide

Get your Nebula Poi up and running in 30 minutes!

## What You'll Need

### Hardware
- [ ] Teensy 4.1 development board
- [ ] ESP32 or ESP32-S3 development board
  - ESP32-DevKitC or similar
  - **ESP32-S3 N16R8 recommended** for new builds (16MB Flash, 8MB PSRAM)
- [ ] APA102 LED strip with 32 LEDs
- [ ] 5V power supply (2-3A)
- [ ] Jumper wires
- [ ] Micro USB cables (2)
- [ ] Optional: Breadboard for prototyping

### Software
- [ ] Arduino IDE (1.8.x or 2.x)
- [ ] Teensyduino addon
- [ ] ESP32 board support
- [ ] Computer with WiFi capability

### Time Required
- Hardware assembly: 15 minutes
- Software installation: 10 minutes
- Testing: 5 minutes

## Step-by-Step Setup

### Step 1: Install Software (10 minutes)

#### A. Install Arduino IDE
1. Download from https://www.arduino.cc/en/software
2. Install following on-screen instructions
3. Launch Arduino IDE

#### B. Install Teensyduino
1. Download from https://www.pjrc.com/teensy/td_download.html
2. Run installer
3. Select Arduino IDE location
4. Install all libraries when prompted

#### C. Install ESP32 Support
1. Open Arduino IDE
2. Go to File > Preferences
3. Add to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to Tools > Board > Boards Manager
5. Search "ESP32"
6. Install "ESP32 by Espressif Systems"

#### D. Install FastLED Library
1. Go to Sketch > Include Library > Manage Libraries
2. Search "FastLED"
3. Install latest version

✅ **Checkpoint**: All software installed

### Step 1.5: Choose Your Firmware (1 minute)

This project offers two firmware options for Teensy 4.1:

#### Option A: Arduino IDE Firmware (Recommended)
- ✅ **Location**: `teensy_firmware/teensy_firmware.ino`
- ✅ Single-file, easy to understand
- ✅ All features including sequences and SD card support
- ✅ **This guide uses this version**

#### Option B: PlatformIO Firmware (Advanced)
- 📁 **Location**: `firmware/teensy41/`
- ⚙️ Modular architecture
- ⚠️ Some features still in development
- 📖 See [FIRMWARE_ARCHITECTURE.md](FIRMWARE_ARCHITECTURE.md) for details

**For this Quick Start, we'll use Option A (Arduino IDE firmware)**

✅ **Checkpoint**: Firmware option selected

### Step 2: Wire the Hardware (15 minutes)

**📌 Important**: The wiring shown below works for **ALL ESP32 variants** including:
- ESP32-WROOM-32 (original ESP32)
- ESP32-DevKitC (most common development board)
- ESP32-S3 (recommended for new builds)

All variants use the same GPIO pins (16/17) - no wiring changes needed between variants!

#### Power Connections
```
5V Power Supply (+) → LED Strip 5V
                    → Teensy VIN
                    → ESP32 VIN

Power Supply (-)    → LED Strip GND
                    → Teensy GND
                    → ESP32 GND
```

#### Teensy to LED Strip
```
Teensy Pin 11 → LED Strip DATA (DI)
Teensy Pin 13 → LED Strip CLOCK (CI)
```

#### Teensy to ESP32/ESP32-S3
```
Teensy Pin 1 (TX1) → ESP32/S3 GPIO 16 (RX2)
Teensy Pin 0 (RX1) → ESP32/S3 GPIO 17 (TX2)
```
**Note**: GPIO 16/17 are standard on all ESP32 variants (WROOM, DevKit, S3) - the same wiring works for all!

**✅ Boot Mode Safety**: GPIO16 and GPIO17 do **NOT** interfere with boot mode. Your ESP32/ESP32-S3 will boot normally with these connections. (Boot mode uses GPIO0 and GPIO46, which are separate pins.)

**Important**: All grounds must be connected together!

✅ **Checkpoint**: Hardware wired correctly

### Step 3: Program Teensy 4.1 (5 minutes)

1. Connect Teensy to computer via USB
2. Open `teensy_firmware/teensy_firmware.ino`
3. Select:
   - Tools > Board > Teensy 4.1
   - Tools > USB Type > Serial
   - Tools > CPU Speed > 600 MHz
4. Click Upload (arrow button)
5. Wait for upload to complete

**Expected Output** (Serial Monitor at 115200 baud):
```
Teensy 4.1 Nebula Poi Initializing...
Teensy 4.1 Nebula Poi Ready!
Commands: IMAGE, PATTERN, SEQUENCE, LIVE, STATUS
```

✅ **Checkpoint**: Teensy programmed and LEDs show startup animation

### Step 4: Program ESP32 or ESP32-S3 (5 minutes)

**For ESP32:**
1. Connect ESP32 to computer via USB
2. Open `esp32_firmware/esp32_firmware.ino`
3. Select:
   - Tools > Board > **ESP32 Dev Module** (works for ESP32-WROOM-32)
   - Tools > Port > [Your ESP32 COM port]
   - Tools > Partition Scheme > **Default** (or "Minimal SPIFFS" if you have 4MB flash)
   - Tools > Flash Size > **4MB (32Mb)** (for ESP32-WROOM-32)
   - Tools > CPU Frequency > **240MHz (WiFi/BT)** (default)
4. Click Upload
5. Wait for upload to complete

**Note for ESP32-WROOM-32**: The "ESP32 Dev Module" board setting works perfectly. If you have a 4MB flash version (most common), use "Default" partition scheme. If you encounter SPIFFS errors, try "Minimal SPIFFS" partition scheme instead.

**For ESP32-S3:**
1. Connect ESP32-S3 to computer via USB
2. Open `esp32_firmware/esp32_firmware.ino`
3. Select:
   - Tools > Board > ESP32S3 Dev Module
   - Tools > Port > [Your ESP32-S3 COM port]
   - Tools > USB CDC On Boot > Enabled
   - Tools > Partition Scheme > 16MB Flash (3MB APP/9.9MB FATFS)
4. Click Upload
5. Wait for upload to complete

**Expected Output** (Serial Monitor at 115200 baud):
```
ESP32 Nebula Poi Controller Starting...
SPIFFS Mounted
Starting Access Point...
AP IP address: 192.168.4.1
Web server started
ESP32 Nebula Poi Controller Ready!
```

✅ **Checkpoint**: ESP32/ESP32-S3 programmed and WiFi AP started

### Step 5: Connect and Test (5 minutes)

#### A. Connect to WiFi
1. On your phone/computer, go to WiFi settings
2. Look for network: **POV-POI-WiFi**
3. Connect using password: **povpoi123**
4. Wait for connection

#### B. Open Web Interface
1. Open web browser
2. Navigate to: **http://192.168.4.1**
3. Web interface should load

#### C. Test Demo Content

The firmware comes pre-loaded with demo content! Try these:

1. **Demo Patterns** (Mode: Pattern Display):
   - Set mode to "Pattern Display"
   - Set index to 0 for Rainbow pattern
   - Try indices 1-4 for Fire, Comet, Breathing, Plasma
   
2. **Demo Images** (Mode: Image Display):
   - Set mode to "Image Display"
   - Set index to 0 for Smiley Face
   - Try indices 1-2 for Rainbow Gradient and Heart
   
3. **Demo Sequence** (Mode: Sequence):
   - Set mode to "Sequence"
   - Set index to 0
   - Watch it cycle through images and patterns!

4. **Brightness Test**:
   - Move brightness slider
   - LEDs should change brightness

✅ **Checkpoint**: System fully operational!

**📖 See [DEMO_CONTENT.md](DEMO_CONTENT.md) for complete details on all built-in content**

## Quick Test Checklist

- [ ] LEDs light up on power
- [ ] Teensy shows startup animation
- [ ] ESP32 creates WiFi network
- [ ] Can connect to POV-POI-WiFi
- [ ] Web interface loads at 192.168.4.1
- [ ] Brightness control works
- [ ] Pattern buttons work
- [ ] LEDs respond to web commands

## Common Issues

### LEDs Don't Light Up
- Check 5V power connection
- Verify LED strip polarity
- Check data/clock pins (11, 13)

### Can't See WiFi Network
- Check ESP32 power
- Wait 10-20 seconds after power on
- Try rebooting ESP32

### Web Interface Won't Load
- Verify connected to POV-POI-WiFi
- Try http://192.168.4.1 directly
- Clear browser cache

### Patterns Don't Change
- Check Teensy-ESP32 serial connection
- Verify RX/TX crossover
- Check common ground

## Next Steps

Now that your system is working:

1. **Learn the Features**
   - Try all pattern types
   - Adjust brightness and frame rate
   - Test live drawing mode

2. **Upload Custom Images**
   - Use the image converter script
   - Upload via web interface
   - Test POV effect by spinning

3. **Try the Android App**
   - Build the Android app from `POVPoiApp/`
   - Control POV device from your phone
   - Convert and upload images on the go
   - See [Android Quick Start](POVPoiApp/QUICKSTART.md)

4. **Read Full Documentation**
   - [Complete Guide](docs/README.md)
   - [API Documentation](docs/API.md)
   - [Wiring Details](docs/WIRING.md)

5. **Build Your POI**
   - Design physical housing
   - Add handle and mounting
   - Consider battery power
   - Add safety features

## Safety Reminders

⚠️ **Before Using**:
- Secure all connections
- Check for loose wires
- Test at low brightness first
- Ensure proper mounting
- Be aware of spinning hazards

## Getting Help

If you encounter issues:
1. Check troubleshooting section
2. Review serial monitor output
3. Verify all connections
4. Test components individually

## Tips for Best Results

💡 **Performance Tips**:
- Start with patterns before images
- Use high contrast images
- Spin at steady speed
- Test in dark environment
- Adjust brightness for battery life

## Congratulations! 🎉

Your Nebula Poi is ready! Start creating amazing light displays!

---

**Need more help?** See the full documentation in the `/docs` folder.
