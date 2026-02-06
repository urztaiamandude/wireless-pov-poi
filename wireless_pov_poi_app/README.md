# Wireless POV Poi - Flutter App

Multi-platform Flutter application for controlling Wireless POV Poi devices via Bluetooth Low Energy (BLE).

## Features

- ✅ **Multi-platform**: Android, Windows, Web (Chrome/Edge)
- ✅ **BLE Control**: Direct Bluetooth connection (no WiFi required)
- ✅ **Pattern Management**: Import, edit, and organize LED patterns
- ✅ **Pattern Creators**: Text, color, gradient, and stacked patterns
- ✅ **Sequencer**: Automated pattern playback
- ✅ **Multi-device**: Control multiple poi simultaneously
- ✅ **APA102 Support**: Optimized for 31-pixel APA102 LED strips

## Supported Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| Android | ✅ Full support | API 21+ (Lollipop) |
| Windows | ✅ Full support | Windows 10/11 with BLE adapter |
| Web | ✅ Limited support | Chrome/Edge only, iOS unsupported |
| iOS | ⚠️ Unsupported | Web Bluetooth not available |

## Installation

### Android

1. Download APK from [Releases](https://github.com/urztaiamandude/wireless-pov-poi/releases)
2. Enable "Install from Unknown Sources" in Android settings
3. Install APK

### Windows

1. Download installer from [Releases](https://github.com/urztaiamandude/wireless-pov-poi/releases)
2. Run `WirelessPOVPoi-Setup.exe`
3. Follow installation wizard

### Web

Visit: [https://urztaiamandude.github.io/wireless-pov-poi-app](https://urztaiamandude.github.io/wireless-pov-poi-app)

## Development Setup

### Prerequisites

- Flutter SDK ≥3.7.2
- Android Studio (for Android builds)
- Visual Studio 2022 (for Windows builds)
- Chrome/Edge (for Web builds)

### Build Commands

```bash
# Get dependencies
flutter pub get

# Run on connected device
flutter run

# Build Android APK
flutter build apk --release

# Build Windows executable
flutter build windows --release

# Build Web app
flutter build web --release
```

## Usage

### Connecting to Poi

1. Power on your Wireless POV Poi device
2. Open the app
3. Tap "Scan for Devices"
4. Select your device from the list
5. Tap "Connect"

### Uploading Patterns

1. Tap "Import" button (+)
2. Select an image (BMP, PNG, or JPG)
3. Pattern will be validated and added to library
4. Tap pattern to upload to poi

**Pattern Requirements:**
- Max height: 31 pixels
- Max width: 400 pixels
- Max total pixels: 40,000
- Format: BMP, PNG, or JPG

### Pattern Operations

- **Delete**: Long-press pattern → "Delete"
- **Flip**: Long-press pattern → "Flip" (vertical mirror)
- **Mirror**: Long-press pattern → "Mirror" (horizontal mirror)
- **Rotate**: Use Pattern Creators → "Rotate Image"

### Pattern Creators

#### Text Pattern
1. Tap "Create" → "Text Pattern"
2. Enter text (capital letters only)
3. Select font size (20, 25, or 55 pixels)
4. Choose foreground and background colors
5. Tap "Save"

#### Color Pattern
1. Tap "Create" → "Color Pattern"
2. Select color
3. Set dimensions
4. Tap "Save"

#### Stacked Pattern
1. Tap "Create" → "Stack Patterns"
2. Select 2+ patterns to combine
3. Patterns will be stacked side-by-side (LCM of widths)
4. Tap "Save"

### Sequencer

Create automated pattern sequences:

1. Tap "Sequencer" button
2. Tap "Add Segment"
3. Configure each segment:
   - Pattern bank (A, B, or C)
   - Pattern slot (1-5)
   - Brightness (0-31)
   - Speed (0-2000ms)
   - Duration (0-20000ms)
4. Tap "Upload to Poi"

**Note**: Sequences run offline on poi after upload.

### Device Settings (Hidden)

Long-press "Wireless POV Poi" title to access:
- Device name
- LED type
- LED count
- Brightness presets (6 levels)
- Speed presets (6 levels)
- Pattern shuffle duration

## Technical Specifications

### BLE Protocol

- **Service UUID**: `6e400001-b5a3-f393-e0a9-e50e24dcca9e` (Nordic UART)
- **RX UUID**: `6e400002-b5a3-f393-e0a9-e50e24dcca9e`
- **TX UUID**: `6e400003-b5a3-f393-e0a9-e50e24dcca9e`
- **Max packet**: 509 bytes
- **Protocol**: See [BLE_PROTOCOL.md](../../BLE_PROTOCOL.md)

### Pattern Format

**Column-major RGB**:
```
for (col = 0; col < width; col++) {
    for (row = 0; row < height; row++) {
        bytes[index++] = red;
        bytes[index++] = green;
        bytes[index++] = blue;
    }
}
```

### Performance

- **Pattern upload**: ~1.8 seconds (31×64 pixels)
- **BLE range**: ~10 meters (clear line of sight)
- **Multi-device**: Up to 7 poi simultaneously

## Troubleshooting

### Device Not Found

- Check poi is powered on
- Enable Bluetooth on phone/computer
- Move closer to poi (<10m)
- Restart app and try again

### Connection Failed

- Check BLE permissions are granted (Android)
- Disable WiFi on poi (if active)
- Restart poi and retry
- Check Bluetooth adapter is BLE 4.0+ (Windows)

### Pattern Upload Failed

- Check pattern dimensions (≤31×400 pixels)
- Check total pixels (≤40,000)
- Move closer to poi
- Reconnect and retry

### Web Version Not Working

- Use Chrome or Edge browser (Firefox/Safari unsupported)
- Enable Web Bluetooth: `chrome://flags/#enable-web-bluetooth`
- Use HTTPS connection (required for Web Bluetooth)
- Note: iOS does not support Web Bluetooth

## Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Submit a pull request

## License

MIT License - see [LICENSE](../../LICENSE)

## Acknowledgments

- Based on [Open-Pixel-Poi](https://github.com/Mitchlol/Open-Pixel-Poi) by Mitchlol
- Nordic UART Service specification
- Flutter Blue Plus package

## Support

- **GitHub Issues**: [Report bugs](https://github.com/urztaiamandude/wireless-pov-poi/issues)
- **Documentation**: [Wiki](https://github.com/urztaiamandude/wireless-pov-poi/wiki)
- **Hardware**: [Hardware documentation](../../README.md)
