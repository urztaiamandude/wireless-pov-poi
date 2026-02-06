# Flutter App Integration Guide

## Overview

This document describes the integration of the Wireless POV Poi Flutter app with the existing hardware and firmware.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Flutter App Layer                         │
│  (Windows / Web)                                                │
│                                                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │   Welcome    │  │     Home     │  │   Pattern    │         │
│  │     Page     │  │     Page     │  │   Database   │         │
│  └──────────────┘  └──────────────┘  └──────────────┘         │
└─────────────────────────────────────────────────────────────────┘
                             │
                             │ BLE (Nordic UART Service)
                             │
┌─────────────────────────────────────────────────────────────────┐
│                      Hardware Layer                              │
│                                                                   │
│  ┌──────────────┐           ┌──────────────┐                   │
│  │   Teensy     │  Serial   │    ESP32     │                   │
│  │    4.1       │◄─────────►│ (BLE Module) │                   │
│  │              │  115200    │              │                   │
│  └──────────────┘           └──────────────┘                   │
│         │                                                        │
│         │                                                        │
│  ┌──────────────┐                                               │
│  │   APA102     │                                               │
│  │  LED Strip   │                                               │
│  │  (31 LEDs)   │                                               │
│  └──────────────┘                                               │
└─────────────────────────────────────────────────────────────────┘
```

## Communication Protocols

### WiFi vs BLE

The Wireless POV Poi system supports **two independent control methods**:

#### WiFi Control (Web Portal)
- **Interface**: ESP32 Access Point + Web Server
- **Protocol**: HTTP REST API
- **Advantages**:
  - Works on any device with WiFi and browser
  - Rich web interface with image upload
  - Live drawing canvas
  - No special app required
- **Use Case**: Primary control method for single-device use

#### BLE Control (Flutter App)
- **Interface**: Direct BLE connection via Flutter app
- **Protocol**: Nordic UART Service (custom protocol)
**Advantages**:
  - Multi-platform app (Windows, Web)
  - Control multiple devices simultaneously
  - Lower latency
  - Offline pattern management
- **Use Case**: Advanced control, multi-device scenarios

### BLE Implementation

The Flutter app communicates with the ESP32 via Bluetooth Low Energy using the Nordic UART Service:

- **Service UUID**: `6e400001-b5a3-f393-e0a9-e50e24dcca9e`
- **RX UUID**: `6e400002-b5a3-f393-e0a9-e50e24dcca9e` (App → Device)
- **TX UUID**: `6e400003-b5a3-f393-e0a9-e50e24dcca9e` (Device → App)

See [BLE_PROTOCOL.md](BLE_PROTOCOL.md) for detailed protocol specification.

## Hardware Requirements

### For BLE Support

The ESP32 must have BLE enabled in firmware:

```cpp
// ESP32 firmware must include:
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Initialize BLE in setup():
BLEDevice::init("Wireless POV Poi");
BLEServer *pServer = BLEDevice::createServer();
// ... setup UART service
```

**Note**: Current ESP32 firmware focuses on WiFi. BLE support needs to be added or coexist with WiFi.

### LED Strip Configuration

- **Total LEDs**: 32 (defined in firmware)
- **Display LEDs**: 31 (LED 0 is level shifter only)
- **Type**: APA102 (SPI-based, supports high refresh rates)
- **Max Pattern Height**: 31 pixels (one per display LED)

## Pattern Format

### Column-Major RGB

All patterns use column-major RGB format for efficient POV rendering:

```
for (col = 0; col < width; col++) {
    for (row = 0; row < height; row++) {
        data[index++] = red;    // 0-255
        data[index++] = green;  // 0-255
        data[index++] = blue;   // 0-255
    }
}
```

This matches the hardware rendering loop which processes one column per frame.

### Pattern Validation

The Flutter app enforces these constraints (defined in `lib/config.dart`):

```dart
static const int MAX_PATTERN_WIDTH = 400;
static const int MAX_PATTERN_HEIGHT = 31;
static const int MAX_PATTERN_PIXELS = 40000;
```

Validation occurs in `lib/database/patterndb.dart` before storing patterns.

## Database Schema

Patterns are stored locally using SQLite:

```sql
CREATE TABLE images (
    id INTEGER PRIMARY KEY,
    height INTEGER,      -- Pattern height (pixels)
    count INTEGER,       -- Pattern width (pixels)
    bytes BLOB          -- RGB data (count * height * 3 bytes)
);
```

Database file: `wireless_pov_patterns.db` (platform-specific location)

## App Structure

### Core Files

```
lib/
├── main.dart                 # App entry point
├── model.dart               # App state (connected devices)
├── config.dart              # Hardware specs and constraints
├── pages/
│   ├── welcome.dart         # BLE device discovery and connection
│   └── home.dart            # Main control interface
├── hardware/
│   ├── ble_uart.dart        # BLE communication layer
│   └── poi_hardware.dart    # Device command interface
└── database/
    ├── dbimage.dart         # Pattern data model
    └── patterndb.dart       # Pattern storage and validation
```

### Platform-Specific Files

```
windows/
└── runner/
    └── main.cpp                              # Entry point + title

web/
├── index.html                                # HTML shell
└── manifest.json                             # PWA manifest
```

## Multi-Device Support

### Scanning and Selection

The welcome page allows selecting multiple devices:

```dart
// Scan for devices with "Wireless POV" in name
await FlutterBluePlus.startScan(
  withKeywords: ["Wireless POV"],
  timeout: Duration(seconds: 5)
);
```

### Connection Management

Each device gets its own `PoiHardware` instance:

```dart
List<PoiHardware> poiList = [];
for (var result in selectedDevices) {
  BLEUart uart = BLEUart(result.device);
  await uart.isIntialized;
  PoiHardware poi = PoiHardware(uart);
  poiList.add(poi);
}
```

### Broadcast Commands

Commands can be sent to all connected devices:

```dart
for (var poi in connectedPoi) {
  await poi.setBrightness(128);
  await poi.setMode(1, 0);  // Display mode
}
```

## Building and Deployment

### Windows

```bash
cd wireless_pov_poi_app
flutter build windows --release
```

Output: `build/windows/x64/runner/Release/`

### Web

```bash
cd wireless_pov_poi_app
flutter build web --release
```

Output: `build/web/`

**Note**: Web version requires HTTPS for Web Bluetooth API.

## Testing

### Without Hardware

The app can be built and tested for UI/UX without actual poi hardware:

```bash
flutter run
```

The welcome page will show no devices during scan, which is expected.

### With Hardware

1. Ensure ESP32 firmware has BLE enabled
2. Device should advertise as "Wireless POV Poi" (or similar)
3. Scan should discover the device
4. Connection should succeed and allow sending commands

### Platform-Specific Testing

- **Windows**: Requires BLE adapter, test on Windows 10/11
- **Web**: Test in Chrome/Edge only (Firefox/Safari unsupported)

## Firmware Integration (Future Work)

### Adding BLE to ESP32 Firmware

The ESP32 firmware currently focuses on WiFi AP mode. To add BLE support:

1. **Dual-mode operation**: ESP32 can run WiFi and BLE simultaneously
2. **Nordic UART Service**: Implement the UART service
3. **Command routing**: Route BLE commands to Teensy via serial
4. **Advertisement**: Advertise as "Wireless POV Poi"

Example structure:

```cpp
// Setup both WiFi and BLE
void setup() {
  // Existing WiFi setup
  WiFi.softAP(ssid, password);
  
  // Add BLE setup
  BLEDevice::init("Wireless POV Poi");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Setup RX characteristic (app writes)
  BLECharacteristic *pRxChar = pService->createCharacteristic(
    RX_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pRxChar->setCallbacks(new BLECallbacks());
  
  // Setup TX characteristic (app reads)
  BLECharacteristic *pTxChar = pService->createCharacteristic(
    TX_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  
  pService->start();
  pServer->getAdvertising()->start();
}

// BLE callback forwards commands to Teensy
class BLECallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) {
    std::string value = pChar->getValue();
    // Forward to Teensy via Serial2
    Serial2.write((uint8_t*)value.c_str(), value.length());
  }
};
```

### Teensy Firmware Changes

No changes required to Teensy firmware. It receives commands via serial regardless of whether they originated from WiFi or BLE.

## Known Limitations

### Platform Support

- **iOS**: Web version doesn't work (no Web Bluetooth API support)
  - Native iOS app would require separate development
- **Linux**: Flutter supports Linux, but BLE stack may need additional setup
- **macOS**: Flutter supports macOS, but not tested

### BLE vs WiFi

- **BLE range**: ~10m (shorter than WiFi but more reliable in some environments)
- **Transfer speed**: BLE is slower than WiFi for large image uploads
- **Simultaneous use**: ESP32 can't do BLE + WiFi AP simultaneously without firmware changes

### Pattern Storage

- Patterns stored locally on device running Flutter app
- Not synced with ESP32/Teensy storage
- Each app instance has independent pattern library

## Future Enhancements

### Firmware Side

1. **Dual-mode ESP32**: WiFi AP + BLE simultaneously
2. **Pattern sync**: Sync patterns between BLE and WiFi interfaces
3. **Device pairing**: Remember paired devices
4. **OTA updates**: Firmware updates via BLE

### App Side

1. **Pattern sync**: Cloud storage for patterns
2. **Pattern editor**: Built-in image editor
3. **Sequencer UI**: Visual sequence editor
4. **Live mode**: Real-time drawing interface
5. **iOS app**: Native iOS version with BLE support
6. **Settings page**: Device configuration (currently hidden)

## Troubleshooting

### Device Not Found

- Check ESP32 has BLE enabled in firmware
- Verify device is advertising as "Wireless POV"
- Check Bluetooth is enabled on host device
- Try moving closer (<10m)

### Connection Failed

- Restart app and retry
- Power cycle the poi device
- Check no other app is connected to device

### Pattern Upload Failed

- Verify pattern dimensions (≤31×400 pixels)
- Check total pixels (≤40,000)
- Try smaller pattern
- Reconnect and retry

### Web Version Not Working

- Use Chrome or Edge (Firefox/Safari unsupported)
- Enable Web Bluetooth: `chrome://flags/#enable-web-bluetooth`
- Ensure HTTPS connection (required for Web Bluetooth)
- Note: iOS does not support Web Bluetooth at all

## References

- **Flutter**: https://flutter.dev
- **Flutter Blue Plus**: https://pub.dev/packages/flutter_blue_plus
- **Nordic UART Service**: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/bluetooth_services/services/nus.html
- **Web Bluetooth API**: https://developer.mozilla.org/en-US/docs/Web/API/Web_Bluetooth_API
- **Open-Pixel-Poi**: https://github.com/Mitchlol/Open-Pixel-Poi

## Contributing

When contributing to the Flutter app:

1. Follow Flutter/Dart style guidelines
2. Test on all target platforms (Windows, Web)
3. Validate pattern dimensions in `patterndb.dart`
4. Update BLE_PROTOCOL.md if protocol changes
5. Keep hardware constraints in `config.dart`
6. Maintain Open-Pixel-Poi attribution in code

## License

This Flutter app is based on Open-Pixel-Poi and maintains the same MIT License. See [LICENSE](../LICENSE) for details.
