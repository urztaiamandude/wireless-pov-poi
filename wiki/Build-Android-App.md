# Android App Build

The Android app lives in `POVPoiApp/`.

## Requirements

- Android Studio Electric Eel (2022.1.1) or later
- Android SDK API 21+
- JDK 8 or later

## Android Studio

1. Open Android Studio.
2. **File > Open** and select the `POVPoiApp/` directory.
3. Wait for Gradle sync to finish.

Build:
- **Build > Make Project** (Ctrl+F9 / Cmd+F9)

Run:
- Select a device or emulator and click **Run**.

## Gradle CLI

```bash
cd POVPoiApp
./gradlew build
./gradlew assembleDebug
```

## Release APK

1. **Build > Generate Signed Bundle / APK**.
2. Choose **APK** and create or select a keystore.
3. Select build variant **release**.
4. APK output: `POVPoiApp/app/release/app-release.apk`

## Common issues

- Missing SDK components: open **SDK Manager** and install required items.
- Gradle sync failures: **File > Invalidate Caches / Restart**.
