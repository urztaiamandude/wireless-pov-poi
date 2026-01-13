# Android App Quick Start Guide

Get the POV POI Controller Android app running in under 10 minutes!

## Prerequisites

- **Computer**: Windows, macOS, or Linux
- **Android Device**: Running Android 5.0 (API 21) or later
- **USB Cable**: For connecting your device

## Step 1: Install Android Studio (5 minutes)

### Download

1. Go to [https://developer.android.com/studio](https://developer.android.com/studio)
2. Click **Download Android Studio**
3. Accept the terms and download

### Install

**Windows:**
- Run the downloaded `.exe` file
- Follow the installation wizard
- Accept default settings

**macOS:**
- Open the downloaded `.dmg` file
- Drag Android Studio to Applications folder
- Launch from Applications

**Linux:**
- Extract the downloaded `.tar.gz` file
- Run `studio.sh` from the `bin/` directory

### First Launch

1. Launch Android Studio
2. Complete the setup wizard
3. Choose **Standard** installation
4. Wait for SDK components to download (~2-3 GB)

## Step 2: Open the Project (1 minute)

### Option A: Using Quick Start Script

**Windows:**
```
double-click: open_android_app.bat
```

**macOS/Linux:**
```bash
./open_android_app.sh
```

### Option B: Manual Open

1. Launch Android Studio
2. Click **Open**
3. Navigate to the repository folder
4. Select the **POVPoiApp** folder
5. Click **OK**

### Wait for Gradle Sync

- First time will take 2-5 minutes
- Downloads dependencies automatically
- Watch the progress bar at the bottom

## Step 3: Prepare Your Android Device (2 minutes)

### Enable Developer Mode

1. Open **Settings** on your Android device
2. Go to **About Phone**
3. Tap **Build Number** 7 times
4. You'll see "You are now a developer!"

### Enable USB Debugging

1. Go back to **Settings**
2. Find **Developer Options** (may be under System)
3. Enable **USB Debugging**
4. Connect device via USB
5. On device, tap **Allow** when prompted for USB debugging

## Step 4: Run the App (2 minutes)

### Build and Install

1. In Android Studio, wait for Gradle sync to finish
2. Your device should appear in the device dropdown (top toolbar)
3. Click the green **Play** button (▶) or press **Shift+F10**
4. Wait for build to complete (~1-2 minutes first time)
5. App will install and launch on your device!

### Verify Installation

The app should open showing:
- App title: "POV POI Controller"
- Status card showing "Disconnected"
- Control sliders
- Pattern buttons

## Step 5: Connect to POV POI Device

### Power On POV Device

1. Ensure your POV POI hardware is powered on
2. Wait ~10 seconds for ESP32 to start WiFi

### Connect to WiFi

1. On your Android device, open **WiFi settings**
2. Look for network: **POV-POI-WiFi**
3. Tap to connect
4. Enter password: **povpoi123**
5. Wait for connection

### Verify in App

1. Return to POV POI Controller app
2. Tap **Refresh Status** button
3. Status should change to "Connected"
4. Mode will display current mode

## Quick Test

### Test Pattern Control

1. Tap the **Rainbow** button
2. POV device LEDs should display rainbow pattern
3. Try other pattern buttons

### Test Brightness

1. Move the **Brightness** slider
2. LEDs should get brighter/dimmer

### Test Image Converter

1. Tap **Image Converter** button
2. Tap **Select from Gallery**
3. Choose a photo
4. Tap **Convert Image**
5. See before/after preview
6. Tap **Upload** to send to POV device

## Troubleshooting

### "Device not detected"

- Check USB cable is connected
- Enable USB debugging in Developer Options
- Try a different USB cable or port
- On Windows, install USB drivers from device manufacturer

### "Gradle sync failed"

- Check internet connection
- Wait and try again (servers may be slow)
- In Android Studio: File → Invalidate Caches / Restart

### "Build failed"

- Ensure you opened the **POVPoiApp** folder (not the parent)
- Try: Build → Clean Project, then Build → Rebuild Project
- Check Java is installed: `java -version` (need Java 8+)

### "App crashes on launch"

- Check device meets minimum Android 5.0 requirement
- View Logcat in Android Studio for error details
- Reinstall the app

### "Can't connect to POV device"

- Ensure you're connected to **POV-POI-WiFi** network
- Check POV hardware is powered on
- Verify ESP32 WiFi AP is running (check ESP32 serial output)
- Try accessing http://192.168.4.1 in mobile browser to verify

### "Image converter not working"

**Camera:**
- Grant camera permission when prompted
- Check device has a working camera

**Gallery:**
- Grant storage/media permission when prompted
- On Android 10+, choose "Allow" for media access

**Upload:**
- Must be connected to POV-POI-WiFi
- Check POV device is responsive
- Try smaller images first

## Next Steps

### Customize the App

- Change colors: Edit `app/src/main/res/values/colors.xml`
- Change strings: Edit `app/src/main/res/values/strings.xml`
- Add features: Modify Kotlin files in `app/src/main/java/com/example/povpoi/`

### Build Release APK

1. Build → Generate Signed Bundle / APK
2. Select APK
3. Create or select keystore
4. Build release
5. Install on other devices

### Learn More

- [Complete Android App Documentation](POVPoiApp/README.md)
- [API Documentation](docs/API.md)
- [Android Developer Guide](https://developer.android.com/docs)

## Tips

- **WiFi Priority**: Android may prefer mobile data. Disable mobile data temporarily for best connection.
- **Power Saving**: Disable battery optimization for the app for better performance.
- **Permissions**: Grant all requested permissions for full functionality.
- **Updates**: Pull latest changes from repository for app updates.

## Success!

You should now have:
- ✓ Android Studio installed and configured
- ✓ POV POI Controller app built and running
- ✓ App connected to your POV device
- ✓ Pattern control working
- ✓ Image converter functional

Enjoy controlling your POV POI system from your Android device! 🎨✨

---

**Having issues?** Check the main [TROUBLESHOOTING.md](TROUBLESHOOTING.md) or open an issue on GitHub.
