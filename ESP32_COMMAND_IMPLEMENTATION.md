# ESP32 Command Processing Implementation Summary

## Overview

This document describes the implementation of ESP32 command processing for the Teensy 4.1 PlatformIO firmware. The implementation adds support for the ESP32's simple serial protocol while maintaining compatibility with the structured message protocol.

## Problem Statement

The ESP32 firmware sends commands using a simple protocol format:
```
[0xFF][CMD][LEN][DATA...][0xFE]
```

However, the Teensy PlatformIO firmware was originally designed with a structured protocol:
```
[TYPE][LENGTH_H][LENGTH_L][DATA...][CHECKSUM]
```

This mismatch prevented the ESP32 from communicating with the PlatformIO Teensy firmware.

## Solution

Implemented **dual protocol support** in the Teensy firmware to handle both protocols:
1. **Simple Protocol**: For ESP32 compatibility (0xFF...0xFE markers)
2. **Structured Protocol**: For future advanced features (checksums, etc.)

The firmware now attempts to read simple protocol messages first, then falls back to structured protocol if needed.

## Implementation Details

### Files Modified

1. **firmware/teensy41/include/esp32_interface.h**
   - Added `LEDDriver` forward declaration
   - Added `setLEDDriver()` method
   - Added `readSimpleMessage()` method for simple protocol
   - Added `processSimpleCommand()` method
   - Added simple protocol handler methods:
     - `handleSimpleImageUpload()`
     - `handleSimplePatternUpload()`
     - `handleSimpleLiveFrame()`

2. **firmware/teensy41/src/esp32_interface.cpp**
   - Added LED driver reference and setter
   - Implemented `readSimpleMessage()` for 0xFF...0xFE protocol
   - Implemented `processSimpleCommand()` dispatcher
   - Implemented `processCommand()` for structured protocol commands
   - Implemented `handleImageData()` for structured image messages
   - Implemented simple protocol handlers for image, pattern, and live frame
   - Added ~300 lines of new code

3. **firmware/teensy41/src/main.cpp**
   - Updated loop to try simple protocol first
   - Falls back to structured protocol
   - Added LED driver reference to ESP32 interface

### Supported Commands

#### Simple Protocol Commands (from ESP32)

| Command | Value | Description | Data Format |
|---------|-------|-------------|-------------|
| Set Mode | 0x01 | Change display mode | [mode][index] |
| Upload Image | 0x02 | Upload POV image | [len_h][len_l][width][height][RGB...] |
| Upload Pattern | 0x03 | Upload pattern data | [index][type][r1][g1][b1][r2][g2][b2][speed] |
| Live Frame | 0x05 | Send live frame data | [31 LEDs * 3 bytes RGB] |
| Set Brightness | 0x06 | Adjust LED brightness | [brightness 0-255] |
| Set Frame Rate | 0x07 | Set frame delay | [delay_ms] |
| Status Request | 0x10 | Request device status | (no data) |

#### Structured Protocol Commands

| Command | Value | Description |
|---------|-------|-------------|
| CMD_PLAY | 0x01 | Start/resume POV display |
| CMD_PAUSE | 0x02 | Pause POV display |
| CMD_STOP | 0x03 | Stop and clear display |
| CMD_SET_BRIGHTNESS | 0x04 | Set brightness |
| CMD_SET_MODE | 0x05 | Set display mode |

### Protocol Flow

```
ESP32 → [0xFF][CMD][LEN][DATA...][0xFE] → Teensy
                                          ↓
                                    readSimpleMessage()
                                          ↓
                                  processSimpleCommand()
                                          ↓
                            ┌─────────────┴─────────────┐
                            ↓                           ↓
                    Handler Functions           LED Driver / POV Engine
                            ↓                           ↓
                    Update LEDs / Display      Update State
```

## Features Implemented

### ✅ Completed

1. **Command Processing**
   - All 5 structured protocol commands (PLAY, PAUSE, STOP, SET_BRIGHTNESS, SET_MODE)
   - All 7 simple protocol commands (0x01-0x07, 0x10)
   - Proper error handling and validation

2. **Image Handling**
   - Direct image upload via simple protocol
   - Image data validation (width × height × 3 bytes)
   - Integration with POV engine

3. **Brightness Control**
   - Direct LED driver control
   - Range validation (0-255)

4. **Live Drawing Mode**
   - Real-time LED updates
   - 31 LED strip support (LED 0 reserved for level shifting)

5. **Status Reporting**
   - Status request handling
   - Response format: [0xFF][0xBB][mode][index][0xFE]

### ⚠️ Partial / TODO

1. **Pattern Generation**
   - Pattern upload handler implemented
   - Pattern generation in POV engine needs implementation
   - Currently just acknowledges receipt of pattern data

2. **Frame Rate Control**
   - Command handler implemented
   - Integration with POV engine frame timing needs implementation

3. **Display Mode Management**
   - Mode setting implemented
   - Index parameter handling needs POV engine support

## Testing Recommendations

### Build Test
```bash
cd firmware/teensy41
pio run -e teensy41
```

### Serial Communication Test

1. **Test Brightness Command**
   ```
   Send: FF 06 01 80 FE
   (Set brightness to 128)
   ```

2. **Test Mode Change**
   ```
   Send: FF 01 02 02 00 FE
   (Set mode to pattern, index 0)
   ```

3. **Test Status Request**
   ```
   Send: FF 10 00 FE
   Expect: FF BB [mode] [index] FE
   ```

4. **Test Live Frame**
   ```
   Send: FF 05 5D [93 bytes RGB data] FE
   (Update 31 LEDs)
   ```

### Integration Test with ESP32

1. Power up system with both ESP32 and Teensy
2. Connect to POV-POI-WiFi network
3. Open web interface at 192.168.4.1
4. Test each control:
   - Brightness slider
   - Mode selection
   - Pattern buttons
   - Image upload
   - Live drawing

## Known Limitations

1. **Pattern Generation**: Pattern upload is accepted but not yet rendered. Requires POV engine pattern system implementation.

2. **Frame Rate**: Frame delay is received but not dynamically applied. POV engine uses fixed frame rate from config.

3. **Mode Index**: Index parameter in mode command is received but not utilized. Needs POV engine support for multiple images/patterns per mode.

4. **Protocol Detection**: Simple protocol is tried first. If a structured protocol message starts with 0xFF, it may be misinterpreted. This is unlikely in practice but worth noting.

## Memory Usage

- Static buffer: 2048 bytes for message receiving
- Image buffer: Dynamic allocation in POV engine (width × height × 3 bytes)
- Pattern data: ~9 bytes per pattern (not yet stored)

## Future Enhancements

1. **Pattern System**: Implement pattern generation in POV engine
   - Rainbow, wave, gradient, sparkle effects
   - Color interpolation
   - Speed control

2. **Dynamic Frame Rate**: Add POV engine support for variable frame timing

3. **Protocol Negotiation**: Add version/capability exchange during startup

4. **Error Recovery**: Add timeout and recovery mechanisms for incomplete messages

5. **Checksum Validation**: Add optional checksums to simple protocol

## Related Documentation

- [FIRMWARE_ARCHITECTURE.md](FIRMWARE_ARCHITECTURE.md) - Overall firmware architecture
- [docs/API.md](docs/API.md) - Web API and protocol details
- [teensy_firmware/README.md](teensy_firmware/README.md) - Arduino IDE version reference

## Version

- Implementation Date: 2026-01-13
- Teensy Firmware: v2.0+ (PlatformIO)
- Protocol Version: Simple Protocol v1 + Structured Protocol v1
- Status: Command processing complete, testing required

---

**Note**: This implementation achieves feature parity with the Arduino IDE version's command processing while maintaining the modular architecture of the PlatformIO version.
