# POV POI Controller - Android App

A complete Android application for controlling the Wireless POV POI system with advanced image conversion capabilities.

## Features

- **POV Device Control**: Control display modes, brightness, and frame rate
- **Quick Patterns**: One-tap access to Rainbow, Wave, Gradient, and Sparkle patterns
- **Image Converter**: Convert photos to POV-compatible format
  - Gallery picker and camera support
  - Real-time before/after preview
  - Adjustable settings (width, height, contrast)
  - Save to gallery or upload directly to device
- **Real-time Status**: Monitor connection and display mode
- **WiFi Integration**: Direct connection to POV-POI-WiFi network

## Requirements

- Android Studio Electric Eel (2022.1.1) or later
- Android SDK API 21+ (Android 5.0 Lollipop or later)
- Android device or emulator
- Java Development Kit (JDK) 8 or later

## Project Setup

### 1. Open in Android Studio

1. Launch Android Studio
2. Select **File → Open**
3. Navigate to the `POVPoiApp` directory
4. Click **OK** to open the project
5. Wait for Gradle sync to complete

### 2. Configure SDK

If prompted:
1. Install any missing SDK components
2. Accept Android SDK licenses
3. Download required build tools

### 3. Build the Project

```bash
# From command line
./gradlew build

# Or use Android Studio
# Build → Make Project (Ctrl+F9 / Cmd+F9)
```

## Running the App

### On Physical Device

1. Enable **Developer Options** on your Android device:
   - Go to Settings → About Phone
   - Tap **Build Number** 7 times
   - Return to Settings → Developer Options
   - Enable **USB Debugging**

2. Connect device via USB

3. In Android Studio:
   - Select your device from the dropdown
   - Click **Run** (Shift+F10 / Ctrl+R)

### On Emulator

1. Create an AVD (Android Virtual Device):
   - Tools → Device Manager
   - Click **Create Device**
   - Select a phone model (e.g., Pixel 4)
   - Select system image (API 21+)
   - Finish setup

2. Launch the emulator and run the app

## Connecting to POV POI Device

### Hardware Setup

1. Power on your POV POI device
2. Wait for ESP32 to create WiFi access point

### WiFi Connection

1. On your Android device:
   - Open **Settings → WiFi**
   - Connect to network: **POV-POI-WiFi**
   - Enter password: **povpoi123**

2. Launch the POV POI Controller app

3. The app will automatically connect to `http://192.168.4.1`

4. Verify connection in the Status section

## Using the App

### Main Controller

**Status Card**
- Shows connection status (Connected/Disconnected)
- Displays current display mode

**Controls**
- **Brightness Slider**: Adjust LED brightness (0-255)
- **Frame Rate Slider**: Adjust display frame rate (10-120 FPS)

**Quick Patterns**
- Tap any pattern button to activate:
  - Rainbow: Cycling rainbow effect
  - Wave: Smooth wave pattern
  - Gradient: Two-color gradient
  - Sparkle: Random sparkle effect

**Actions**
- **Image Converter**: Open image conversion tool
- **Refresh Status**: Update connection status

### Image Converter

1. **Select Source**:
   - **Gallery**: Choose from existing photos
   - **Camera**: Take a new photo

2. **Adjust Settings**:
   - Width: 10-100 pixels (default: 31px for POV display)
   - Max Height: 10-128 pixels (default: 64px)
   - Enhance Contrast: Toggle for better POV visibility

3. **Convert**: Process the image

4. **Actions**:
   - **Save**: Save converted image to gallery
   - **Upload**: Send directly to POV device
   - **Share**: Share with other apps

## Project Structure

```
POVPoiApp/
├── app/
│   ├── src/
│   │   └── main/
│   │       ├── java/com/example/povpoi/
│   │       │   ├── MainActivity.kt              # Main controller
│   │       │   ├── ImageConverterActivity.kt    # Image converter
│   │       │   └── POVPoiAPI.kt                 # API client
│   │       ├── res/
│   │       │   ├── layout/
│   │       │   │   ├── activity_main.xml
│   │       │   │   └── activity_image_converter.xml
│   │       │   ├── values/
│   │       │   │   ├── strings.xml
│   │       │   │   ├── colors.xml
│   │       │   │   └── themes.xml
│   │       │   └── xml/
│   │       │       └── file_paths.xml
│   │       └── AndroidManifest.xml
│   ├── build.gradle                             # App dependencies
│   └── proguard-rules.pro                       # ProGuard rules
├── gradle/                                       # Gradle wrapper
├── build.gradle                                  # Project build config
├── settings.gradle                               # Project settings
└── README.md                                     # This file
```

## Permissions

The app requires the following permissions:

- **Internet**: API communication with POV device
- **WiFi State**: Check WiFi connection
- **Camera**: Take photos for conversion
- **Storage** (Android 9 and below): Save images
- **Media Images** (Android 13+): Gallery access

Permissions are requested at runtime when needed.

## Building Release APK

### Generate Signed APK

1. **Build → Generate Signed Bundle / APK**

2. Select **APK**

3. **Create new keystore** (first time):
   - Choose location and password
   - Fill in certificate details
   - Remember the passwords!

4. Or **Use existing keystore**

5. Select build variant: **release**

6. Click **Finish**

7. APK will be in `app/release/app-release.apk`

### Install on Device

```bash
# Via ADB
adb install app/release/app-release.apk

# Or transfer APK to device and install manually
```

## Troubleshooting

### Gradle Sync Failed

**Solution**:
- Check internet connection
- File → Invalidate Caches / Restart
- Update Gradle: File → Project Structure → Project

### Build Errors

**Missing SDK components**:
- Open SDK Manager (Tools → SDK Manager)
- Install required SDK versions

**Dependency conflicts**:
```bash
./gradlew clean
./gradlew build --refresh-dependencies
```

### App Crashes

**Check logs**:
```bash
adb logcat | grep POVPoi
```

Or use Android Studio's Logcat panel

**Common Issues**:
- **Not connected to POV-POI-WiFi**: App needs WiFi connection
- **Permissions denied**: Grant required permissions in Settings
- **Network timeout**: Check POV device is powered on

### Image Converter Issues

**Camera not working**:
- Grant camera permission
- Check if device has a camera

**Can't save images** (Android 9 and below):
- Grant storage permission
- Check available storage space

**Upload fails**:
- Verify WiFi connection to POV-POI-WiFi
- Check POV device is powered and responsive
- Try smaller images

## Customization

### Change POV Device IP

Edit `POVPoiAPI.kt`:
```kotlin
class POVPoiAPI(
    private val baseUrl: String = "http://192.168.4.1"  // Change IP here
)
```

### Modify Color Theme

Edit `res/values/colors.xml`:
```xml
<color name="pov_primary">#FF6200EE</color>
<color name="pov_accent">#FF03DAC5</color>
```

### Add Custom Patterns

Edit `MainActivity.kt` to add more pattern buttons and handlers.

### Adjust Image Converter Settings

Edit `ImageConverterActivity.kt`:
```kotlin
// Default conversion width
private var targetWidth = 31  // Change default width

// Default max height
private var maxHeight = 64    // Change default height
```

## API Documentation

The app uses REST API endpoints:

- `GET /api/status` - Get system status
- `POST /api/mode` - Set display mode
- `POST /api/brightness` - Set brightness
- `POST /api/framerate` - Set frame rate
- `POST /api/pattern` - Send pattern configuration
- `POST /api/image` - Upload image
- `POST /api/live` - Send live frame data

See main repository's [API Documentation](../docs/API.md) for details.

## Development

### Code Style

This project follows Kotlin coding conventions:
- Use meaningful variable names
- Keep functions focused and short
- Add comments for complex logic
- Use coroutines for async operations

### Testing

Run unit tests:
```bash
./gradlew test
```

Run instrumented tests:
```bash
./gradlew connectedAndroidTest
```

### Debugging

1. Set breakpoints in Android Studio
2. Run in debug mode (Shift+F9 / Ctrl+D)
3. Use Logcat for runtime logs
4. Use Layout Inspector for UI debugging

## Version History

- **1.0.0** (2024):
  - Initial release
  - Main controller with status and controls
  - Quick pattern buttons
  - Image converter with gallery and camera support
  - Real-time preview and contrast enhancement
  - Save and upload functionality

## Support

For issues, questions, or contributions:
1. Check the main repository documentation
2. Review troubleshooting section
3. Open an issue on GitHub

## License

This project is open source and available under the same license as the main POV POI project.

## Credits

Built with:
- Kotlin programming language
- AndroidX libraries
- Material Components for Android
- OkHttp for networking
- Kotlin Coroutines for async operations

---

**Enjoy controlling your POV POI system!** 🎨✨
