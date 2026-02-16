# Teensy 4.1 PSRAM Installation Guide

This guide covers installing external PSRAM chips on your Teensy 4.1 to dramatically increase available memory for POV images and patterns.

## Overview

The Teensy 4.1 has two footprints on the bottom for optional PSRAM chips. With PSRAM installed, the firmware can store:

- **Without PSRAM**: 10 images at 32×200 pixels (~60KB total)
- **With 16MB PSRAM** (2× 8MB chips): 50 images at 32×400 pixels (~1.8MB total, only 11% of PSRAM)

This allows storing many more images and larger POV images for better visual quality.

## Required Materials

### PSRAM Chips
- **Recommended**: 2× IPS6404L-SQ-SPN 8MB PSRAM chips
- **Alternative compatible chips**:
  - APS6404L-3SQR-SN
  - ESP-PSRAM64H (64Mbit/8MB)
  - Any PSRAM chip with the same pinout

### Tools
- Fine-tipped soldering iron (recommended: temperature-controlled)
- Good quality solder (63/37 or 60/40 tin-lead recommended)
- Flux (rosin-based or no-clean)
- Magnifying glass or microscope
- Isopropyl alcohol (90%+) for cleaning
- Lint-free wipes or cotton swabs

### Optional but Recommended
- Hot air rework station (for easier installation)
- Solder paste
- Desoldering braid or solder sucker (for corrections)
- ESD-safe workspace

## Installation Steps

### 1. Prepare the Teensy Board

1. **Clean the PSRAM pads** on the bottom of the Teensy 4.1
   - Use isopropyl alcohol to remove any oils or residue
   - Let dry completely

2. **Apply flux** to all PSRAM pads
   - This helps solder flow and prevents bridging

### 2. Position the PSRAM Chip

The PSRAM chips are very small (SOIC-8 package), so careful positioning is critical:

1. **Orient the chip correctly**
   - Pin 1 indicator (dot or line) should match the silkscreen on the Teensy
   - Refer to Teensy 4.1 documentation for pad layout

2. **Align all pins**
   - Use magnification to ensure all pins align with pads
   - The chip should sit flat and centered

### 3. Soldering Methods

#### Method A: Soldering Iron (Manual)

1. **Tack one corner pin** first to hold the chip in place
2. **Check alignment** - adjust if needed while only one pin is soldered
3. **Solder remaining pins** one at a time
   - Use minimal solder on the iron tip
   - Touch each pin briefly (1-2 seconds)
   - Avoid bridging between pins

4. **Inspect for bridges**
   - Use magnification to check between all pins
   - Remove bridges with desoldering braid if needed

#### Method B: Hot Air (Recommended if available)

1. **Apply solder paste** to all pads
2. **Position the chip** carefully
3. **Heat with hot air** at 350-380°C
   - Move in circular motion to distribute heat evenly
   - Watch for solder to flow (paste will become shiny)
   - Remove heat immediately when solder flows

4. **Let cool naturally** - don't move the board

### 4. Install Second Chip

Repeat the process for the second PSRAM chip on the other footprint.

### 5. Clean the Board

1. **Remove excess flux** with isopropyl alcohol
2. **Inspect solder joints** under magnification
3. **Check for shorts** between adjacent pins with a multimeter

### 6. Testing

1. **Connect Teensy to computer** via USB

2. **Upload test sketch** or main firmware

3. **Open Serial Monitor** at 115200 baud

4. **Check startup message**:
   ```
   PSRAM detected: 16 MB
   Image capacity: 50 images at 400x32 max
   ```

If PSRAM is not detected, check:
- Chip orientation (pin 1 position)
- Solder joints (reflow if necessary)
- No bridges between pins
- Chip is genuine and compatible

## Firmware Configuration

The firmware automatically detects PSRAM at startup. No configuration changes are needed - the code uses preprocessor directives to enable PSRAM features when `ARDUINO_TEENSY41` is defined.

### Key Changes with PSRAM:

```cpp
// Automatically configured in teensy_firmware.ino:
#ifdef ARDUINO_TEENSY41
  #define MAX_IMAGES 50              // vs 10 without PSRAM
  #define IMAGE_MAX_WIDTH 400        // vs 200 without PSRAM
  #define CMD_BUFFER_SIZE 40000      // vs 6400 without PSRAM
  EXTMEM POVImage images[MAX_IMAGES]; // Stored in PSRAM
#endif
```

## Memory Usage

With PSRAM installed, the memory layout is:

| Region | Size | Usage |
|--------|------|-------|
| Internal RAM | 1 MB | Code, stack, small buffers |
| PSRAM Chip 1 | 8 MB | Image storage |
| PSRAM Chip 2 | 8 MB | Image storage |
| **Total PSRAM** | **16 MB** | **~1.8 MB used for 50 images** |

The firmware uses the `EXTMEM` keyword to place large arrays in PSRAM:
- Image pixel arrays
- Command buffer for serial uploads
- Other large data structures

## Troubleshooting

### PSRAM Not Detected

**Symptoms**: Serial monitor shows "PSRAM detected: NONE"

**Solutions**:
1. Check chip orientation - pin 1 must be correctly aligned
2. Reflow solder joints - ensure good connection on all pins
3. Check for solder bridges between pins
4. Verify chip part number matches compatible types
5. Try installing only one chip first to isolate issues

### Partial PSRAM Detection

**Symptoms**: Only 8 MB detected instead of 16 MB

**Solutions**:
1. Check the second chip installation
2. Verify no cold solder joints on second chip
3. Ensure both chips are the same type/speed

### Upload/Runtime Errors

**Symptoms**: Code won't upload or crashes at runtime

**Solutions**:
1. Reduce MAX_IMAGES if testing without PSRAM
2. Check power supply can handle increased current
3. Verify no shorts to ground or adjacent pins
4. Test with simpler sketch to isolate PSRAM vs code issues

## Performance Notes

- **PSRAM Access Speed**: Slower than internal RAM (~2-3x slower)
- **Impact**: Minimal for this application - image data is read sequentially
- **Best Practice**: Keep frequently-accessed variables in internal RAM

## Safety Notes

⚠️ **ESD Precaution**: PSRAM chips are sensitive to static electricity
- Use ESD wrist strap when handling chips
- Work on ESD-safe mat if available
- Touch grounded metal before handling chips

⚠️ **Heat Sensitive**: Don't overheat the chips
- Maximum temperature: 260°C for 10 seconds
- Let board cool between rework attempts
- Avoid heating same area repeatedly

## Resources

- [Teensy 4.1 PSRAM Product Page](https://www.pjrc.com/store/psram.html)
- [PJRC Memory Test Sketch](https://github.com/PaulStoffregen/teensy41_extram)
- [Teensy 4.1 Pinout Reference](https://www.pjrc.com/teensy/card11a_rev4.pdf)

## Support

If you have issues with PSRAM installation:
1. Post to [PJRC Forums](https://forum.pjrc.com/)
2. Check GitHub Issues for this project
3. Join the project Discord (link in main README)

---

**Last Updated**: 2025-02-16  
**Author**: Wireless POV Poi Development Team
