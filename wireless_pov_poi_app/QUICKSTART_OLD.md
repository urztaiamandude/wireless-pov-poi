# Quick Start Guide - Flutter App Development

## Prerequisites

Before you begin, ensure you have:

1. **Flutter SDK** ≥3.7.2
   - Download: https://docs.flutter.dev/get-started/install
   - Verify: `flutter doctor`

2. **Platform-Specific Tools**
   - **Windows**: Visual Studio 2022 with C++ workload
   - **Web**: Chrome or Edge browser

3. **BLE Hardware** (for testing)
   - Wireless POV Poi device with BLE enabled
   - Or ESP32 dev board for testing

## Initial Setup

### 1. Install Dependencies

```bash
cd wireless_pov_poi_app
flutter pub get
```

This will download all required packages:
- flutter_blue_plus (BLE)
- provider (state management)
- sqflite (database)
- image (processing)
- And more...

### 2. Verify Installation

```bash
flutter doctor -v
```

Check for:
- ✅ Flutter (Channel stable)
- ✅ Visual Studio (if building for Windows)
- ✅ Chrome (if building for Web)

### 3. Add Sample Patterns (Optional)

Place 10 BMP files in `patterns/` directory:
- Naming: `pattern1.bmp` through `pattern10.bmp`
- Size: Any width × 31 pixels height
- Format: 24-bit RGB BMP

These will be loaded into the database on first run.

## Running the App

### Development Mode

```bash
# Connect device or start emulator, then:
flutter run

# Or select specific platform:
flutter run -d windows
flutter run -d chrome
```

### Hot Reload

While app is running:
- Press `r` to hot reload (fast)
- Press `R` to hot restart (slower, full restart)
- Press `q` to quit

## Building for Production

### Windows Executable

```bash
flutter build windows --release

# Output: build/windows/x64/runner/Release/
```

Package as installer (optional):
- Use Inno Setup or NSIS
- Include Visual C++ Redistributable

### Web App

```bash
flutter build web --release

# Output: build/web/
```

Deploy to web server:
```bash
# Example: Deploy to GitHub Pages
cp -r build/web/* /path/to/gh-pages/
```

**Important**: Web Bluetooth requires HTTPS!

## Testing Without Hardware

### UI Testing

The app UI can be tested without BLE hardware:

1. Welcome page will show empty device list
2. This is expected behavior
3. UI/UX can still be validated

### Mock Data

To test with mock data, modify `lib/pages/welcome.dart`:

```dart
// Add fake device for testing
setState(() {
  // Create mock scan result (pseudo-code)
  // This requires creating mock BLE objects
});
```

Or use Flutter integration tests with mocked BLE.

## Common Development Tasks

### Adding a New Command

1. Add command code in `lib/hardware/poi_hardware.dart`:
   ```dart
   static const int CMD_NEW_FEATURE = 0x30;
   ```

2. Implement command method:
   ```dart
   Future<void> sendNewFeature(int param) async {
     List<int> packet = [0xFF, CMD_NEW_FEATURE, param, 0xFE];
     await uart.write(packet);
   }
   ```

3. Update BLE_PROTOCOL.md documentation

### Adding a New Page

1. Create file: `lib/pages/newpage.dart`

2. Define StatefulWidget:
   ```dart
   import 'package:flutter/material.dart';
   
   class NewPage extends StatefulWidget {
     const NewPage({super.key});
     
     @override
     _NewPageState createState() => _NewPageState();
   }
   
   class _NewPageState extends State<NewPage> {
     @override
     Widget build(BuildContext context) {
       return Scaffold(
         appBar: AppBar(title: Text("New Feature")),
         body: Container(),
       );
     }
   }
   ```

3. Navigate from home page:
   ```dart
   Navigator.push(
     context,
     MaterialPageRoute(builder: (context) => NewPage()),
   );
   ```

### Modifying Pattern Validation

Edit `lib/database/patterndb.dart`, method `insertImage()`:

```dart
// Change max dimensions
if (image.height > 64) {  // Changed from 31
  throw Exception("Height must be ≤64");
}
```

Don't forget to update `lib/config.dart` constants!

### Adding Pattern Operations

In `lib/database/patterndb.dart`:

```dart
Future<void> rotateImage90(int id) async {
  var image = await getDBImage(id);
  // Implement 90-degree rotation
  // ...
  insertImage(rotatedImage);
  clearInMemoryCache();
}
```

## Debugging

### Enable Verbose Logging

```dart
// In lib/hardware/ble_uart.dart
print("BLE: Connecting to ${device.platformName}");
print("BLE: Discovered services: ${services.length}");
```

### Flutter DevTools

```bash
flutter run
# Then open DevTools URL shown in console
```

Features:
- Widget inspector
- Network monitor
- Performance profiler
- Memory analyzer

### BLE Debugging
## Troubleshooting

### "Package not found"

```bash
flutter pub get
flutter pub upgrade
```

### "BLE not working" (Windows)

- Check BLE adapter is installed
- Install BLE drivers
- Run as Administrator (first time)

### "Web Bluetooth not available"

- Use Chrome or Edge (not Firefox/Safari)
- Enable Web Bluetooth: `chrome://flags/#enable-web-bluetooth`
- Ensure HTTPS connection

### "Database error"

Delete and recreate database:
```dart
// In lib/database/patterndb.dart
await deleteDatabase(
  join(await getDatabasesPath(), 'wireless_pov_patterns.db')
);
```

## Code Style

### Dart Formatting

```bash
# Format all Dart files
flutter format .

# Check formatting
flutter format --set-exit-if-changed .
```

### Linting

```bash
# Run static analysis
flutter analyze

# Fix auto-fixable issues
dart fix --apply
```

### Best Practices

1. **Use const constructors** where possible
2. **Await async operations** properly
3. **Handle errors** with try-catch
4. **Dispose resources** in dispose()
5. **Use meaningful names** for variables
6. **Comment complex logic**
7. **Update documentation** with changes

## Project Structure Guidelines

```
lib/
├── main.dart              # Keep minimal, just app setup
├── model.dart            # Global state only
├── config.dart           # Constants, no logic
├── pages/                # Full-screen pages
│   ├── welcome.dart
│   ├── home.dart
│   └── settings.dart     # To be implemented
├── widgets/              # Reusable widgets (to be created)
│   └── pattern_card.dart
├── hardware/             # Hardware abstraction
│   ├── ble_uart.dart
│   └── poi_hardware.dart
├── database/             # Data persistence
│   ├── dbimage.dart
│   └── patterndb.dart
└── utils/                # Utility functions (to be created)
    └── image_converter.dart
```

## Resources

### Official Documentation

- **Flutter**: https://docs.flutter.dev
- **Dart**: https://dart.dev/guides
- **flutter_blue_plus**: https://pub.dev/packages/flutter_blue_plus

### Community

- **Flutter Discord**: https://discord.gg/flutter
- **Stack Overflow**: Tag `flutter`
- **GitHub Issues**: Report bugs here

### Learning

- **Flutter Codelabs**: https://docs.flutter.dev/codelabs
- **Widget of the Week**: https://www.youtube.com/playlist?list=PLjxrf2q8roU23XGwz3Km7sQZFTdB996iG
- **Flutter in Focus**: https://www.youtube.com/playlist?list=PLjxrf2q8roU2HdJQDjJzOeO6J3FoFLWr2

## Next Steps

After basic setup:

1. **Implement Pattern Creators**
   - Text pattern UI
   - Color pattern UI
   - Gradient pattern UI
   - Stack pattern UI

2. **Implement Sequencer**
   - Visual timeline
   - Segment editor
   - Playback controls

3. **Implement Settings**
   - Device configuration
   - Brightness presets
   - Speed presets

4. **Add Image Import**
   - File picker
   - Image processing
   - Preview and crop

5. **Add Live Mode**
   - Drawing canvas
   - Real-time updates

6. **Polish UI**
   - Custom theme
   - Animations
   - Transitions

## Contributing

When contributing:

1. Fork the repository
2. Create feature branch: `git checkout -b feature/new-feature`
3. Make changes following code style
4. Test on all platforms
5. Update documentation
6. Submit pull request

## License

This project is based on Open-Pixel-Poi (MIT License) and adapted for Wireless POV Poi.

See [LICENSE](../LICENSE) for details.
