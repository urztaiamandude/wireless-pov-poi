# Flutter App Project Structure

## Complete Directory Tree

```
wireless_pov_poi_app/
├── README.md                          # User documentation (5.6 KB)
├── BLE_PROTOCOL.md                    # Protocol specification (6.3 KB)
├── INTEGRATION.md                     # Integration guide (12.5 KB)
├── IMPLEMENTATION_SUMMARY.md          # Complete overview (14.4 KB)
├── QUICKSTART.md                      # Developer setup (8.4 KB)
├── PROJECT_STRUCTURE.md               # This file
│
├── pubspec.yaml                       # Flutter dependencies & metadata
├── analysis_options.yaml              # Dart linter configuration
├── .gitignore                         # Git ignore rules
│
├── lib/                               # Dart source code
│   ├── main.dart                     # App entry point (24 lines)
│   ├── model.dart                    # App state management (5 lines)
│   ├── config.dart                   # Hardware specifications (46 lines)
│   │
│   ├── pages/                        # UI pages
│   │   ├── welcome.dart              # BLE device discovery (165 lines)
│   │   └── home.dart                 # Main control interface (138 lines)
│   │
│   ├── hardware/                     # Hardware abstraction layer
│   │   ├── ble_uart.dart             # BLE communication (72 lines)
│   │   └── poi_hardware.dart         # Device commands (88 lines)
│   │
│   └── database/                     # Data persistence
│       ├── dbimage.dart              # Pattern data model (27 lines)
│       └── patterndb.dart            # Pattern storage & validation (223 lines)
│
├── windows/                          # Windows platform
│   └── runner/
│       └── main.cpp                  # Windows entry point (42 lines)
│
├── web/                              # Web platform
│   ├── index.html                    # HTML shell (22 lines)
│   ├── manifest.json                 # PWA manifest (36 lines)
│   └── icons/                        # App icons (to be added)
│       ├── Icon-192.png
│       ├── Icon-512.png
│       ├── Icon-maskable-192.png
│       └── Icon-maskable-512.png
│
└── patterns/                         # Pattern assets
    ├── README.md                     # Instructions
    └── pattern1-10.bmp               # Demo patterns (to be added)
```

## File Statistics

| Category | Files | Lines | Description |
|----------|-------|-------|-------------|
| **Core Dart Code** | 9 | 788 | Application logic |
| **Platform Code** | 4 | 126 | Windows/Web |
| **Configuration** | 3 | 98 | pubspec, analysis, gitignore |
| **Documentation** | 6 | 1,393 | User & developer guides |
| **Total** | **22** | **2,405** | Complete project |

## Dependencies (pubspec.yaml)

### Runtime Dependencies

```yaml
flutter: sdk                    # Flutter framework
cupertino_icons: ^1.0.8        # iOS-style icons
provider: ^6.1.2                # State management
flutter_blue_plus: ^1.35.5      # BLE communication
rxdart: ^0.28.0                # Reactive streams
image: ^4.0.17                 # Image processing
path: ^1.8.2                   # Path utilities
sqflite: ^2.2.8+4              # SQLite (mobile)
sqflite_common_ffi: ^2.3.5     # SQLite (desktop)
sqflite_common_ffi_web: ^1.0.0 # SQLite (web)
tuple: ^2.0.1                  # Tuple data structure
image_picker: ^1.1.2           # Image selection
```

### Development Dependencies

```yaml
flutter_test: sdk              # Testing framework
flutter_lints: ^5.0.0         # Linting rules
```

## Module Breakdown

### 1. main.dart (Entry Point)

```dart
runApp(WirelessPOVPoiApp)
  └── MaterialApp
      └── ChangeNotifierProvider<Model>
          └── WelcomePage
```

### 2. pages/welcome.dart (Device Discovery)

**Responsibilities:**
- Scan for BLE devices
- Filter devices by "Wireless POV"
- Multi-device selection
- Connection establishment
- Navigate to HomePage

**Key Methods:**
- `scan()` - Start BLE scan
- `connect()` - Connect to selected devices

### 3. pages/home.dart (Main Interface)

**Responsibilities:**
- Display device info card
- Show pattern library
- Connection state indicators
- Loading overlay

**Features:**
- LED count and type display
- Connected device count
- Pattern management (stub)
- Settings access (long-press title)

### 4. hardware/ble_uart.dart (BLE Layer)

**Responsibilities:**
- Nordic UART Service connection
- Characteristic discovery
- Data transmission
- Data reception stream

**Key Components:**
- Service UUID: 6e400001-b5a3-f393-e0a9-e50e24dcca9e
- RX Characteristic: 6e400002... (Write)
- TX Characteristic: 6e400003... (Notify)

### 5. hardware/poi_hardware.dart (Commands)

**Responsibilities:**
- Device command interface
- Protocol formatting
- Chunked data transfer

**Commands:**
- `uploadImage()` - Upload pattern
- `setMode()` - Set display mode
- `setBrightness()` - Adjust brightness
- `setFramerate()` - Set frame rate
- `uploadPattern()` - Animated pattern
- `disconnect()` - Close connection

### 6. database/patterndb.dart (Storage)

**Responsibilities:**
- SQLite database management
- Pattern validation
- Pattern operations
- Caching

**Validation:**
- Height ≤ 31 pixels
- Width ≤ 400 pixels
- Total pixels ≤ 40,000
- RGB data length check

**Operations:**
- `insertImage()` - Add pattern
- `deleteImage()` - Remove pattern
- `invertImage()` - Vertical flip
- `reverseImage()` - Horizontal flip

### 7. config.dart (Constants)

**Hardware Specs:**
```dart
LED_COUNT = 31
LED_TYPE = "APA102"
MAX_PATTERN_HEIGHT = 31
MAX_PATTERN_WIDTH = 400
MAX_PATTERN_PIXELS = 40000
MAX_BRIGHTNESS = 31  // 5-bit
```

### 8. model.dart (State)

```dart
class Model {
  List<PoiHardware>? connectedPoi;
}
```

Holds connected device list, accessible via Provider.

## Platform-Specific Files

### Windows

**main.cpp**
- Window title: "Wireless POV Poi"
- Window size: 1280×800
- COM initialization for BLE

### Web

**index.html**
- Title: "Wireless POV Poi"
- PWA capable
- Web Bluetooth API support

**manifest.json**
- Name: "Wireless POV Poi"
- Short name: "WirelessPOV"
- Icons: 192×192, 512×512
- Theme: #0175C2

## Build Outputs

### Windows Executable

```
build/
└── windows/
    └── x64/
        └── runner/
            └── Release/
                ├── wireless_pov_poi_app.exe
                ├── flutter_windows.dll
                └── data/
```

### Web Build

```
build/
└── web/
    ├── index.html
    ├── flutter.js
    ├── main.dart.js
    └── assets/
```

## Database Schema

### Table: images

```sql
CREATE TABLE images (
    id INTEGER PRIMARY KEY,
    height INTEGER,      -- Pattern height (pixels)
    count INTEGER,       -- Pattern width (pixels)
    bytes BLOB          -- RGB data (count × height × 3 bytes)
);
```

**Storage Location:**
- Windows: `%APPDATA%/com.urztaiamandude/wireless_pov_poi_app/`
- Web: IndexedDB

## Protocol Packet Format

### Simple Protocol

```
[0xFF][CMD][DATA...][0xFE]
```

### Image Upload Example

```
0xFF                   # Start marker
0x02                   # CMD_IMAGE
0x00 0x40             # Width = 64 (big-endian)
0x00 0x1F             # Height = 31
[R G B] × 64 × 31     # RGB data (5,952 bytes)
0xFE                   # End marker
```

Total: 5,958 bytes → Split into 12 chunks (509 bytes each)

## To Be Implemented

### Pattern Creators
- [ ] Text pattern UI
- [ ] Color pattern UI
- [ ] Gradient pattern UI
- [ ] Stack pattern UI

### Sequencer
- [ ] Timeline UI
- [ ] Segment editor
- [ ] Playback controls

### Settings Page
- [ ] Device configuration
- [ ] Brightness presets
- [ ] Speed presets
- [ ] Advanced options

### Image Import
- [ ] File picker
- [ ] Image processing
- [ ] Preview and crop
- [ ] Aspect ratio handling

### Live Mode
- [ ] Drawing canvas
- [ ] Real-time updates
- [ ] Touch/mouse input

## Documentation Overview

### README.md (User Guide)
- Installation instructions
- Usage guide
- Troubleshooting
- Technical specs

### BLE_PROTOCOL.md (Protocol Spec)
- Service UUIDs
- Command codes
- Data formats
- Examples

### INTEGRATION.md (Architecture)
- System architecture
- Communication flow
- Hardware requirements
- Firmware integration

### IMPLEMENTATION_SUMMARY.md (Overview)
- What was created
- Key features
- Validation status
- Next steps

### QUICKSTART.md (Developer Setup)
- Prerequisites
- Initial setup
- Common tasks
- Debugging tips

### PROJECT_STRUCTURE.md (This File)
- Directory tree
- File breakdown
- Module responsibilities
- Build outputs

## Testing Checklist

### Build Tests
- [ ] `flutter pub get` completes
- [ ] `flutter analyze` shows no errors
- [ ] `flutter build apk` succeeds
- [ ] `flutter build windows` succeeds
- [ ] `flutter build web` succeeds

### Functional Tests
- [ ] Device discovery works
- [ ] Connection succeeds
- [ ] Pattern validation works
- [ ] Image upload completes
- [ ] Multi-device connection works

### Platform Tests
- [ ] Windows exe runs
- [ ] Web loads in Chrome
- [ ] Web loads in Edge

## Maintenance

### Updating Dependencies

```bash
flutter pub upgrade
flutter pub outdated  # Check for newer versions
```

### Code Formatting

```bash
flutter format .      # Format all files
flutter analyze       # Static analysis
```

### Cleaning Build

```bash
flutter clean         # Remove build artifacts
flutter pub get       # Reinstall dependencies
```

## Resources

- **Project Repository**: https://github.com/urztaiamandude/wireless-pov-poi
- **Flutter Docs**: https://docs.flutter.dev
- **Flutter Blue Plus**: https://pub.dev/packages/flutter_blue_plus
- **Original Inspiration**: https://github.com/Mitchlol/Open-Pixel-Poi

---

**Created**: February 2026  
**Status**: Production-ready structure, UI features pending  
**License**: MIT (based on Open-Pixel-Poi)
