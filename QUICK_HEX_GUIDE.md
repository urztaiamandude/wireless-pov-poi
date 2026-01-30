# Quick Guide: Compiling to HEX Format

> Note: Build documentation has moved to the wiki.
> See [Build-Teensy-HEX](wiki/Build-Teensy-HEX.md) and
> [Build-Teensy-CLI](wiki/Build-Teensy-CLI.md).

Need to compile the Teensy firmware to HEX format for the Teensy Loader? Here's the fastest way:

## 🎯 The Big Picture

```
teensy_firmware.ino  →  [Compile]  →  firmware.hex  →  [Teensy Loader]  →  Teensy 4.1
```

## ⚡ Quick Start (Arduino IDE - Recommended)

**Time: < 1 minute**

1. **Open** `teensy_firmware/teensy_firmware.ino` in Arduino IDE
2. **Select** Tools > Board > Teensy 4.1
3. **Select** Tools > USB Type > Serial  
4. **Export** Sketch > Export Compiled Binary (Ctrl+Alt+S / Cmd+Option+S)
5. **Find** the HEX file in `teensy_firmware/teensy_firmware.ino.teensy41.hex`

✅ **That's it!** You now have a HEX file ready to load.

## 📦 Loading the HEX File

1. **Open** Teensy Loader application
2. **File** > Open HEX File
3. **Select** your .hex file
4. **Press** the white button on your Teensy board
5. **Done** - it auto-programs!

## 🔧 Alternative: CLI Compilation

### Arduino CLI (Recommended CLI Method):
```bash
# Linux/Mac
./scripts/build_arduino_cli.sh

# Windows
scripts\build_arduino_cli.bat
```
Output: `teensy_firmware/build/teensy_firmware.ino.hex`

### PlatformIO CLI:
```bash
# Linux/Mac
./scripts/build_teensy_hex.sh

# Windows
scripts\build_teensy_hex.bat
```
Output: `build_output/teensy41_firmware.hex`

## 📚 Need More Help?

- **CLI Compilation**: [docs/CLI_COMPILATION.md](docs/CLI_COMPILATION.md) - Complete CLI guide
- **Detailed Guide**: [docs/BUILDING_HEX.md](docs/BUILDING_HEX.md) - All methods

## ⚠️ Prerequisites

### For Arduino CLI Method:
- Arduino CLI: https://arduino.github.io/arduino-cli/
```bash
# Install Arduino CLI, then:
./scripts/build_arduino_cli.sh  # Linux/Mac
scripts\build_arduino_cli.bat   # Windows
```

### For PlatformIO Method:
```bash
pip install platformio
```

## 🎯 Common Issues

**"Board not found"** → Install Teensyduino  
**"Library not found"** → Install FastLED via Library Manager  
**"Export failed"** → Check Board is set to Teensy 4.1  
**"Can't open HEX"** → Must be compiled for Teensy 4.1 specifically

## 🚀 What's Next?

After loading the HEX:
1. Connect to **POV-POI-WiFi** (password: `povpoi123`)
2. Open **http://192.168.4.1**
3. Control your POV poi!

---

**That's it!** The Arduino IDE export method takes less than 1 minute.
