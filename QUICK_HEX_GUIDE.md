# Quick Guide: Compiling to HEX Format

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

## 🔧 Alternative: PlatformIO Build

### Linux/Mac:
```bash
./scripts/build_teensy_hex.sh
```

### Windows:
```cmd
scripts\build_teensy_hex.bat
```

Output: `build_output/teensy41_firmware.hex`

## 📚 Need More Help?

See the detailed guide: [docs/BUILDING_HEX.md](docs/BUILDING_HEX.md)

## ⚠️ Prerequisites

### For Arduino IDE Method:
- Arduino IDE 1.8.x or 2.x
- Teensyduino addon: https://www.pjrc.com/teensy/td_download.html
- FastLED library (via Library Manager)

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
