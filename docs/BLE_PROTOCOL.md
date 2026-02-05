# BLE Protocol Documentation

## Overview

The Wireless POV Poi uses Bluetooth Low Energy (BLE) for communication between Flutter apps (Android/Windows/Web) and the POI hardware. The ESP32 co-processor implements the Nordic UART Service (NUS) and acts as a BLE-to-UART bridge, translating BLE commands to the internal Teensy protocol.

## Architecture

```
Flutter App (Android/Windows/Web)
         ↓ BLE (Nordic UART Service)
    ESP32 Co-Processor (BLE Bridge)
         ↓ Serial UART (translated protocol)
    Teensy 4.1 (LED Controller)
         ↓ SPI
    APA102 LED Strip (31 pixels)
```

## Nordic UART Service (NUS)

The BLE implementation uses the Nordic UART Service, which provides a simple serial-like interface over BLE.

### Service UUID
```
6e400001-b5a3-f393-e0a9-e50e24dcca9e
```

### Characteristics

| UUID | Type | Description |
|------|------|-------------|
| `6e400002-b5a3-f393-e0a9-e50e24dcca9e` | RX | Write commands from app to device |
| `6e400003-b5a3-f393-e0a9-e50e24dcca9e` | TX | Read responses from device to app (with notify) |

### Device Name
```
"Wireless POV Poi"
```

This name appears during BLE scanning and is used by Flutter apps to discover the device.

## BLE Protocol Format

### Message Structure

All BLE messages use the following format:

```
[0xD0] [command_code] [data_bytes...] [0xD1]
```

- **Start Marker**: `0xD0`
- **Command Code**: 1 byte (see command table below)
- **Data**: Variable length (0-506 bytes)
- **End Marker**: `0xD1`

### Maximum Packet Size

BLE has a maximum packet size of **509 bytes**. For larger transfers (e.g., images), the data should be split into multiple packets.

## Command Reference

### Command Codes

| Code | Name | Description |
|------|------|-------------|
| 0x00 | CC_SUCCESS | Success response from device |
| 0x01 | CC_ERROR | Error response from device |
| 0x02 | CC_SET_BRIGHTNESS | Set global brightness (0-255) |
| 0x03 | CC_SET_SPEED | Set POV rotation speed / frame rate |
| 0x04 | CC_SET_PATTERN | Upload pattern data |
| 0x05 | CC_SET_PATTERN_SLOT | Select pattern slot (1-15) |
| 0x06 | CC_SET_PATTERN_ALL | Enable auto-pattern cycling |
| 0x0E | CC_SET_SEQUENCER | Upload pattern sequence |
| 0x0F | CC_START_SEQUENCER | Start sequencer playback |

### Command Details

#### 0x02 - CC_SET_BRIGHTNESS

Sets the global LED brightness for APA102 LEDs.

**Format:**
```
[0xD0] [0x02] [brightness] [0xD1]
```

**Parameters:**
- `brightness`: 0-255 (0 = off, 255 = full brightness)

**Example:** Set brightness to 128 (50%)
```
0xD0 0x02 0x80 0xD1
```

**Response:**
```
[0xD0] [0x00] [0xD1]  // Success
```

---

#### 0x03 - CC_SET_SPEED

Sets the POV rotation speed / frame rate.

**Format:**
```
[0xD0] [0x03] [speed] [0xD1]
```

**Parameters:**
- `speed`: Frame delay in milliseconds (lower = faster)
  - Range: 1-255
  - Typical: 10-20ms for fast rotation, 30-50ms for slow

**Example:** Set speed to 20ms (50 FPS)
```
0xD0 0x03 0x14 0xD1
```

**Response:**
```
[0xD0] [0x00] [0xD1]  // Success
```

---

#### 0x04 - CC_SET_PATTERN

Uploads a pattern to the device. The pattern is stored in RAM and can be saved to SD card.

**Format:**
```
[0xD0] [0x04] [pattern_data...] [0xD1]
```

**Pattern Data Structure:**
```
[type] [color1_r] [color1_g] [color1_b] [color2_r] [color2_g] [color2_b] [speed]
```

**Parameters:**
- `type`: Pattern type (0-15)
  - Basic: 0=rainbow, 1=wave, 2=gradient, 3=sparkle, 4=fire, 5=comet
  - 6=breathing, 7=strobe, 8=meteor, 9=wipe, 10=plasma
  - Music: 11=VU meter, 12=pulse, 13=rainbow, 14=center, 15=sparkle
- `color1`: Primary color (RGB, 3 bytes)
- `color2`: Secondary color (RGB, 3 bytes)
- `speed`: Animation speed (1-255)

**Example:** Upload rainbow pattern with speed 50
```
0xD0 0x04 0x00 0xFF 0x00 0x00 0x00 0xFF 0x00 0x32 0xD1
```

**Response:**
```
[0xD0] [0x00] [0xD1]  // Success
```

---

#### 0x05 - CC_SET_PATTERN_SLOT

Selects and displays a specific pattern from the available slots.

**Format:**
```
[0xD0] [0x05] [slot] [0xD1]
```

**Parameters:**
- `slot`: Pattern slot number (0-15)

**Example:** Select pattern slot 3
```
0xD0 0x05 0x03 0xD1
```

**Response:**
```
[0xD0] [0x00] [0xD1]  // Success
```

---

#### 0x06 - CC_SET_PATTERN_ALL

Enables auto-pattern cycling mode, which automatically cycles through all available patterns.

**Format:**
```
[0xD0] [0x06] [0xD1]
```

**Parameters:** None

**Example:** Enable auto-cycling
```
0xD0 0x06 0xD1
```

**Response:**
```
[0xD0] [0x00] [0xD1]  // Success
```

---

#### 0x0E - CC_SET_SEQUENCER

Uploads a sequence of patterns/images with durations.

**Format:**
```
[0xD0] [0x0E] [count] [item1] [duration1_msb] [duration1_lsb] ... [0xD1]
```

**Parameters:**
- `count`: Number of sequence items (1-10)
- `itemN`: Pattern/image index
- `durationN`: Duration in milliseconds (16-bit, MSB first)

**Example:** Sequence with 2 patterns (pattern 0 for 1000ms, pattern 1 for 500ms)
```
0xD0 0x0E 0x02 0x00 0x03 0xE8 0x01 0x01 0xF4 0xD1
```

**Response:**
```
[0xD0] [0x00] [0xD1]  // Success
```

---

#### 0x0F - CC_START_SEQUENCER

Starts playback of the uploaded sequence.

**Format:**
```
[0xD0] [0x0F] [sequence_index] [0xD1]
```

**Parameters:**
- `sequence_index`: Sequence slot number (0-4)

**Example:** Start sequence 0
```
0xD0 0x0F 0x00 0xD1
```

**Response:**
```
[0xD0] [0x00] [0xD1]  // Success
```

---

## Response Format

All commands return a response using the same protocol format:

**Success:**
```
[0xD0] [0x00] [0xD1]
```

**Error:**
```
[0xD0] [0x01] [error_code] [0xD1]
```

Error codes (TBD - to be defined in future versions).

## Implementation Notes

### ESP32 BLE Bridge

The ESP32 acts as a bridge between BLE and the Teensy UART:

1. **Receives BLE command** (0xD0...0xD1 format)
2. **Translates to internal protocol** (0xFF...0xFE format)
3. **Forwards via Serial UART** to Teensy
4. **Receives response** from Teensy
5. **Translates back to BLE protocol**
6. **Sends via BLE notify** to app

### Command Translation

The BLE bridge translates BLE command codes to internal Teensy command codes:

| BLE Code | Internal Code | Description |
|----------|--------------|-------------|
| 0x02 | 0x06 | Brightness |
| 0x03 | 0x07 | Speed/Frame rate |
| 0x04 | 0x03 | Pattern upload |
| 0x05 | 0x01 | Set mode (pattern) |
| 0x06 | 0x01 | Set mode (auto-cycle) |
| 0x0E | 0x04 | Sequence upload |
| 0x0F | 0x01 | Set mode (sequence) |

### Connection Management

- The device advertises as "Wireless POV Poi"
- Only one BLE connection is supported at a time
- WiFi and BLE can coexist on ESP32
- Connection timeout: Device automatically restarts advertising after disconnect

### Performance

- **Pattern upload time**: < 2 seconds for 31×64 pixel pattern
- **BLE range**: ~10 meters (typical indoor environment)
- **Command latency**: < 50ms typical

## Flutter Integration

### Example Code (Flutter Blue Plus)

```dart
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

// UUIDs
final serviceUuid = Guid("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
final rxUuid = Guid("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
final txUuid = Guid("6e400003-b5a3-f393-e0a9-e50e24dcca9e");

// Scan for device
FlutterBluePlus.startScan(timeout: Duration(seconds: 4));

FlutterBluePlus.scanResults.listen((results) {
  for (ScanResult r in results) {
    if (r.device.name == "Wireless POV Poi") {
      // Found the device!
      connectToDevice(r.device);
    }
  }
});

// Connect and discover services
Future<void> connectToDevice(BluetoothDevice device) async {
  await device.connect();
  List<BluetoothService> services = await device.discoverServices();
  
  for (BluetoothService service in services) {
    if (service.uuid == serviceUuid) {
      var characteristics = service.characteristics;
      for (BluetoothCharacteristic c in characteristics) {
        if (c.uuid == rxUuid) {
          rxCharacteristic = c;
        } else if (c.uuid == txUuid) {
          txCharacteristic = c;
          // Enable notifications
          await c.setNotifyValue(true);
        }
      }
    }
  }
}

// Send command
Future<void> setBrightness(int brightness) async {
  List<int> cmd = [0xD0, 0x02, brightness, 0xD1];
  await rxCharacteristic.write(cmd);
}

// Listen for responses
txCharacteristic.value.listen((value) {
  // value is List<int> containing response
  if (value.length >= 3 && value[0] == 0xD0 && value[2] == 0xD1) {
    if (value[1] == 0x00) {
      print("Success!");
    } else {
      print("Error: ${value[1]}");
    }
  }
});
```

## Testing

### Test Commands

Use a BLE terminal app (e.g., Nordic nRF Connect) to test:

1. **Scan for device**: Look for "Wireless POV Poi"
2. **Connect**: Establish BLE connection
3. **Test brightness**: Send `D0 02 80 D1` to set 50% brightness
4. **Test pattern**: Send `D0 05 00 D1` to select pattern 0

## Troubleshooting

### Device not found in scan
- Ensure ESP32 is powered on
- Check that BLE is enabled in firmware (`BLE_ENABLED=1`)
- Device may already be connected to another app

### Commands not working
- Verify command format (must start with 0xD0, end with 0xD1)
- Check that Teensy is powered and connected to ESP32
- Enable serial debug output to see command processing

### Connection drops
- Check distance (BLE range ~10m)
- Reduce WiFi interference
- Ensure stable power supply

## References

- **Nordic UART Service**: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/bluetooth_services/services/nus.html
- **ESP32 BLE Arduino**: https://github.com/nkolban/ESP32_BLE_Arduino
- **Flutter Blue Plus**: https://pub.dev/packages/flutter_blue_plus
- **Open-Pixel-Poi Protocol**: https://github.com/Mitchlol/Open-Pixel-Poi

## License

This protocol documentation is part of the Wireless POV Poi project and is licensed under the same terms as the main project.
