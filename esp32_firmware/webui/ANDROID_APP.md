# Android App — Nebula POV Poi

The Nebula POV Poi Android app wraps the React web UI in a native shell using [Capacitor](https://capacitorjs.com/), adding native device features that enhance the POV poi experience.

## Features

### All Web UI Features (via WebView)
- **Dashboard** — real-time brightness, mode, pattern and frame-rate control
- **Pattern Preview** — animated 18-pattern visualiser
- **Image Lab** — upload / generate images, build sequences and deploy to hardware
- **Advanced Settings** — LED hardware config, WiFi management
- **Wiring Guide, Code Viewers, Firmware Manager**

### Native-Only Enhancements
These features are available only when running inside the Android app:

| Feature | Description |
|---------|-------------|
| **Camera Capture** | Take a photo with the device camera and send it straight to Image Lab for POV conversion |
| **Gallery Pick** | Select an existing photo from the gallery |
| **Haptic Feedback** | Tactile vibration on mode changes, pattern selection and toolbar actions |
| **Keep Screen Awake** | Prevent the screen from dimming during a live performance (Wake Lock) |
| **Orientation Lock** | Lock to portrait while controlling poi |
| **Network Indicator** | Real-time WiFi / cellular status badge |
| **Share** | Share project link via the native share sheet |
| **Dark Status Bar** | Status bar styled to match the slate-900 app theme |
| **Native Toast** | Lightweight native notification toasts |

All native features degrade gracefully — the same React code runs in a regular browser with those buttons simply hidden.

## Prerequisites

| Requirement | Version |
|-------------|---------|
| Node.js | 22+ |
| npm | 9+ |
| Android Studio | Latest (Hedgehog or newer) |
| JDK | 21 |
| Android SDK | API 36 (compile), API 24 minimum |

## Quick Start

```bash
# 1. Install web dependencies
cd esp32_firmware/webui
npm install

# 2. Build the production web bundle
npm run build

# 3. Sync web assets + Capacitor plugins into the Android project
npx cap sync android

# 4. Open in Android Studio
npx cap open android
```

In Android Studio, click **Run ▸ Run 'app'** to deploy to a connected device or emulator.

### One-liner Build + Sync

```bash
npm run native:android
```

## Project Structure

```
esp32_firmware/webui/
├── android/                   # Native Android project (Capacitor-managed)
│   ├── app/
│   │   ├── src/main/
│   │   │   ├── AndroidManifest.xml
│   │   │   ├── java/com/nebulapoi/mobile/
│   │   │   │   └── MainActivity.java
│   │   │   └── res/            # Icons, splash, layouts
│   │   └── build.gradle
│   ├── build.gradle
│   ├── variables.gradle        # SDK versions, dependency versions
│   └── gradle/
├── capacitor.config.ts         # Capacitor plugin config
├── nativeFeatures.ts           # Unified native API with web fallbacks
├── components/
│   └── NativeToolbar.tsx       # Floating action button toolbar
└── package.json                # Capacitor plugin dependencies
```

## Architecture

```
┌─────────────────────────────────────────────────┐
│  Android App (Capacitor Shell)                   │
│  ┌─────────────────────────────────────────────┐ │
│  │  WebView (React UI)                         │ │
│  │  ┌───────────┐  ┌───────────┐  ┌─────────┐ │ │
│  │  │ Dashboard  │  │ Image Lab │  │ Pattern │ │ │
│  │  │           │  │  + Camera  │  │ Preview │ │ │
│  │  └───────────┘  └───────────┘  └─────────┘ │ │
│  │  ┌───────────────────────────────────────┐  │ │
│  │  │       NativeToolbar (FAB)             │  │ │
│  │  │  Camera · KeepAwake · Share · etc.    │  │ │
│  │  └───────────────────────────────────────┘  │ │
│  └──────────────────────┬──────────────────────┘ │
│                         │ Capacitor Bridge        │
│  ┌──────────────────────┴──────────────────────┐ │
│  │  Native Plugins                              │ │
│  │  Camera · Haptics · Network · Preferences    │ │
│  │  Share · StatusBar · Toast · Orientation      │ │
│  └─────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────┘
          │ WiFi (HTTP)
          ▼
   ESP32-S3 (192.168.4.1)
          │ UART
          ▼
   Teensy 4.1 (POV Engine)
          │ SPI
          ▼
   APA102 LED Strip
```

## Native Features API

All native capabilities are exposed through `nativeFeatures.ts`:

```typescript
import {
  isNativePlatform,  // true inside the Android/iOS app
  takePhoto,         // opens camera → returns data-URL
  pickPhoto,         // opens gallery → returns data-URL
  hapticImpact,      // light / medium / heavy vibration
  hapticNotification,// success / warning / error vibration
  getNetworkStatus,  // { connected: boolean, connectionType: string }
  onNetworkChange,   // register listener → returns unsubscribe
  keepAwakeOn,       // prevent screen sleep
  keepAwakeOff,      // re-enable screen sleep
  shareContent,      // native share sheet
  showToast,         // native toast notification
  lockPortrait,      // lock to portrait
  unlockOrientation, // allow rotation
  setDarkStatusBar,  // set status bar to match app theme
  savePreference,    // persistent key-value storage
  loadPreference,
} from './nativeFeatures';
```

## Android Permissions

The following permissions are declared in `AndroidManifest.xml`:

| Permission | Purpose |
|------------|---------|
| `INTERNET` | HTTP API calls to ESP32 |
| `CAMERA` | Photo capture for POV images |
| `READ_MEDIA_IMAGES` | Gallery access for image import |
| `VIBRATE` | Haptic feedback |
| `ACCESS_NETWORK_STATE` | Network status monitoring |
| `ACCESS_WIFI_STATE` | WiFi connection detection |
| `WAKE_LOCK` | Keep screen on during performance |

## Networking

The app communicates with the ESP32 over local WiFi:

1. Connect your phone to the **POV-POI-WiFi** network (password: `povpoi123`)
2. The app reaches the ESP32 at `http://192.168.4.1`
3. Cleartext HTTP traffic is enabled in the manifest (`usesCleartextTraffic`)
4. Capacitor config has `allowMixedContent: true` for the Android WebView

## Building a Release APK

```bash
# 1. Build + sync
npm run native:android

# 2. In Android Studio:
#    Build > Generate Signed Bundle / APK > APK
#    Select release build variant
#    Sign with your keystore
```

Or via command line:

```bash
cd android
./gradlew assembleRelease
# APK: android/app/build/outputs/apk/release/app-release-unsigned.apk
```

## Customisation

### App Icon
Replace the launcher icons in `android/app/src/main/res/mipmap-*/` directories with your own assets. Use [Android Asset Studio](https://romannurik.github.io/AndroidAssetStudio/) to generate all density variants.

### Splash Screen
Replace `android/app/src/main/res/drawable*/splash.png` files. The background colour is set to `#020617` (slate-950) in `capacitor.config.ts`.

### App Name
Edit `android/app/src/main/res/values/strings.xml`:
```xml
<string name="app_name">Nebula POV Poi</string>
```

## Troubleshooting

### "No connection to device"
- Ensure phone is on **POV-POI-WiFi** network
- Verify ESP32 is powered and serving at `192.168.4.1`

### Camera not working
- Grant camera permission when prompted
- Check **Settings > Apps > Nebula POV Poi > Permissions > Camera**

### Haptics not working
- Some devices do not have a vibration motor (e.g. tablets)
- The `hapticImpact` / `hapticNotification` calls silently no-op

### Build errors after dependency changes
```bash
cd esp32_firmware/webui
rm -rf node_modules
npm install
npx cap sync android
```
