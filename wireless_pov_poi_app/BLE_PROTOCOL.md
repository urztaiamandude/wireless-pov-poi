# BLE Protocol Documentation

## Overview

The Wireless POV Poi uses BLE (Bluetooth Low Energy) with the Nordic UART Service for communication between the Flutter app and the poi hardware.

## Service UUIDs

- **Service UUID**: `6e400001-b5a3-f393-e0a9-e50e24dcca9e` (Nordic UART Service)
- **RX Characteristic**: `6e400002-b5a3-f393-e0a9-e50e24dcca9e` (Write)
- **TX Characteristic**: `6e400003-b5a3-f393-e0a9-e50e24dcca9e` (Notify)

## Protocol Format

### Simple Protocol (Currently Implemented)

```
[START_MARKER][COMMAND][DATA...][END_MARKER]

START_MARKER: 0xFF
END_MARKER: 0xFE
```

### Command Codes

| Command | Code | Description | Data Format |
|---------|------|-------------|-------------|
| MODE | 0x01 | Set display mode | `[mode:1][index:1]` |
| IMAGE | 0x02 | Upload image pattern | `[width_h:1][width_l:1][height_h:1][height_l:1][rgb_data:N]` |
| PATTERN | 0x03 | Set animated pattern | `[type:1][color1_r:1][color1_g:1][color1_b:1][color2_r:1][color2_g:1][color2_b:1][speed:1]` |
| LIVE_FRAME | 0x05 | Send live frame | `[rgb_data:93]` (31 pixels × 3 bytes) |
| BRIGHTNESS | 0x06 | Set brightness | `[level:1]` (0-255) |
| FRAMERATE | 0x07 | Set frame rate | `[fps:1]` (10-120) |
| STATUS | 0x10 | Request status | None |
| SD_SAVE | 0x20 | Save to SD card | `[slot:1][rgb_data:N]` |
| SD_LOAD | 0x21 | Load from SD card | `[slot:1]` |
| SD_LIST | 0x22 | List SD files | None |
| SD_DELETE | 0x23 | Delete SD file | `[slot:1]` |

### Display Modes

| Mode | Value | Description |
|------|-------|-------------|
| IDLE | 0 | LEDs off or idle pattern |
| IMAGE | 1 | Display uploaded image |
| PATTERN | 2 | Display animated pattern |
| SEQUENCE | 3 | Play pattern sequence |
| LIVE | 4 | Live drawing mode |

### Pattern Types

| Type | Value | Description |
|------|-------|-------------|
| RAINBOW | 0 | Rainbow sweep |
| WAVE | 1 | Sine wave |
| GRADIENT | 2 | Color gradient |
| SPARKLE | 3 | Random sparkles |
| FIRE | 4 | Fire effect |
| COMET | 5 | Comet trail |
| BREATHING | 6 | Breathing effect |
| STROBE | 7 | Strobe flash |
| METEOR | 8 | Meteor shower |
| WIPE | 9 | Color wipe |
| PLASMA | 10 | Plasma effect |
| VU_METER | 11 | Audio VU meter (requires mic) |
| PULSE | 12 | Audio pulse (requires mic) |
| AUDIO_RAINBOW | 13 | Audio-reactive rainbow (requires mic) |
| CENTER_BURST | 14 | Audio center burst (requires mic) |
| AUDIO_SPARKLE | 15 | Audio sparkles (requires mic) |

## Image Upload Protocol

### Image Data Format

Images are stored in **column-major RGB format**:

```
For each column (0 to width-1):
    For each row (0 to height-1):
        RGB[index++] = red   (0-255)
        RGB[index++] = green (0-255)
        RGB[index++] = blue  (0-255)
```

### Image Constraints

- **Max width**: 400 pixels
- **Max height**: 31 pixels (LED strip length)
- **Max total pixels**: 40,000 (width × height)
- **Data size**: width × height × 3 bytes

### Upload Sequence

1. Send START_MARKER (0xFF)
2. Send CMD_IMAGE (0x02)
3. Send width (2 bytes, big-endian)
4. Send height (2 bytes, big-endian)
5. Send RGB data (column-major format)
6. Send END_MARKER (0xFE)

### Chunked Transfer

Due to BLE MTU limitations (typically 512 bytes), large images are sent in chunks:

- **Max chunk size**: 509 bytes
- **Delay between chunks**: 50ms (recommended)
- App automatically handles chunking

Example:
```dart
const int maxChunkSize = 509;
for (int i = 0; i < packet.length; i += maxChunkSize) {
  int end = min(i + maxChunkSize, packet.length);
  await uart.write(packet.sublist(i, end));
  await Future.delayed(Duration(milliseconds: 50));
}
```

## Pattern Upload Protocol

### Pattern Data Format

```
[0xFF][0x03][type][color1_R][color1_G][color1_B][color2_R][color2_G][color2_B][speed][0xFE]
```

- **type**: Pattern type (0-15)
- **color1**: Primary color RGB (3 bytes)
- **color2**: Secondary color RGB (3 bytes)
- **speed**: Animation speed (0-255, higher = slower)

## Live Mode Protocol

### Live Frame Format

```
[0xFF][0x05][R0][G0][B0][R1][G1][B1]...[R30][G30][B30][0xFE]
```

- Send 31 RGB triplets (93 bytes total)
- Frame rate: Up to 60 FPS
- LED index 0 is skipped (level shifter only)
- Display LEDs: 1-31

## Status Response

When STATUS command (0x10) is sent, device responds:

```
[mode:1][index:1][brightness:1][framerate:1][connected:1]
```

## Error Handling

### Common Errors

- **Invalid packet**: Missing start/end markers
- **Checksum failure**: Data corruption
- **Size mismatch**: Image dimensions don't match data length
- **Out of range**: Parameter exceeds valid range
- **Timeout**: No response within 5 seconds

### Recovery

1. Retry command (max 3 attempts)
2. Reconnect BLE if persistent failure
3. Reset device if unresponsive

## Performance Notes

- **Typical upload time**: ~1.8 seconds for 31×64 pixel image
- **BLE range**: ~10 meters clear line of sight
- **Connection stability**: Auto-reconnect on disconnect
- **Multi-device**: Up to 7 poi simultaneously (BLE limitation)

## Implementation Example

See `lib/hardware/poi_hardware.dart` for complete implementation:

```dart
Future<void> uploadImage(Uint8List imageData, int width, int height) async {
  List<int> packet = [];
  packet.add(0xFF); // Start marker
  packet.add(CMD_IMAGE);
  
  // Width and height (2 bytes each)
  packet.add((width >> 8) & 0xFF);
  packet.add(width & 0xFF);
  packet.add((height >> 8) & 0xFF);
  packet.add(height & 0xFF);
  
  // Image data
  packet.addAll(imageData);
  packet.add(0xFE); // End marker
  
  // Send in chunks
  const int maxChunkSize = 509;
  for (int i = 0; i < packet.length; i += maxChunkSize) {
    int end = (i + maxChunkSize < packet.length) ? i + maxChunkSize : packet.length;
    await uart.write(packet.sublist(i, end));
    await Future.delayed(Duration(milliseconds: 50));
  }
}
```

## Future Enhancements

Potential protocol improvements:

- Add CRC checksum for data integrity
- Implement ACK/NACK responses
- Add compression for large images
- Support for image metadata (name, tags)
- Batch upload multiple patterns
- OTA firmware updates via BLE

## References

- Nordic UART Service Specification
- Flutter Blue Plus documentation: https://pub.dev/packages/flutter_blue_plus
- Original Open-Pixel-Poi protocol: https://github.com/Mitchlol/Open-Pixel-Poi
