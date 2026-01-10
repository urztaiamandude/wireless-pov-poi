# Android App Example for POV POI Control

This is a simple example Android app for controlling the wireless POV POI system.

## Overview

This example demonstrates how to build an Android app that communicates with the POV POI system using the REST API.

## Features

- Connect to POV POI WiFi
- Display system status
- Control display modes
- Adjust brightness and frame rate
- Send pattern configurations
- Live drawing mode

## Prerequisites

- Android Studio
- Android SDK (API 21+)
- Kotlin support
- Device with WiFi capability

## Project Structure

```
app/
├── src/
│   └── main/
│       ├── java/com/example/povpoi/
│       │   ├── MainActivity.kt
│       │   ├── POVPoiAPI.kt
│       │   ├── PatternFragment.kt
│       │   └── LiveDrawFragment.kt
│       ├── res/
│       │   ├── layout/
│       │   │   ├── activity_main.xml
│       │   │   └── fragment_pattern.xml
│       │   └── values/
│       │       ├── strings.xml
│       │       └── colors.xml
│       └── AndroidManifest.xml
└── build.gradle
```

## Setup Instructions

### 1. Create New Android Project

1. Open Android Studio
2. Create New Project
3. Select "Empty Activity"
4. Language: Kotlin
5. Minimum SDK: API 21 (Android 5.0)

### 2. Add Dependencies

Add to `app/build.gradle`:

```gradle
dependencies {
    implementation 'androidx.core:core-ktx:1.9.0'
    implementation 'androidx.appcompat:appcompat:1.6.1'
    implementation 'com.google.android.material:material:1.9.0'
    implementation 'androidx.constraintlayout:constraintlayout:2.1.4'
    
    // Network
    implementation 'com.squareup.okhttp3:okhttp:4.11.0'
    implementation 'com.google.code.gson:gson:2.10.1'
    
    // Coroutines
    implementation 'org.jetbrains.kotlinx:kotlinx-coroutines-android:1.7.1'
    
    // ViewModel
    implementation 'androidx.lifecycle:lifecycle-viewmodel-ktx:2.6.1'
    implementation 'androidx.lifecycle:lifecycle-livedata-ktx:2.6.1'
}
```

### 3. Add Permissions

Add to `AndroidManifest.xml`:

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android">
    
    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.ACCESS_WIFI_STATE" />
    <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
    
    <application
        ...>
        ...
    </application>
</manifest>
```

## Code Examples

### POVPoiAPI.kt

See the included `POVPoiAPI.kt` file for the complete API client implementation.

Key methods:
- `getStatus()` - Get current system status
- `setMode(mode, index)` - Set display mode
- `setBrightness(brightness)` - Adjust brightness
- `setPattern(config)` - Upload pattern configuration
- `sendLiveFrame(pixels)` - Send live frame data

### MainActivity.kt

See the included `MainActivity.kt` file for the main activity implementation.

Features:
- Status monitoring
- Mode selection
- Brightness control
- Pattern selection
- Navigation to live draw mode

## Usage

### Connecting to POV POI

1. Install app on Android device
2. Connect device to "POV-POI-WiFi" network
   - Settings > WiFi > POV-POI-WiFi
   - Password: povpoi123
3. Launch app
4. App automatically connects to 192.168.4.1

### Controlling Patterns

```kotlin
// Set rainbow pattern
val api = POVPoiAPI()
api.setPattern(PatternConfig(
    index = 0,
    type = 0, // Rainbow
    color1 = Color(255, 0, 0),
    color2 = Color(0, 0, 255),
    speed = 50
))

// Switch to pattern mode
api.setMode(2, 0)
```

### Adjusting Brightness

```kotlin
// Set brightness to 75%
val api = POVPoiAPI()
api.setBrightness(192)
```

### Live Drawing

```kotlin
// Send custom LED colors
val pixels = List(31) { index ->
    RGBColor(
        r = (index * 8).coerceAtMost(255),
        g = 0,
        b = 255 - (index * 8).coerceAtMost(255)
    )
}

val api = POVPoiAPI()
api.sendLiveFrame(pixels)
```

## Building and Installing

### Debug Build

1. Connect Android device via USB
2. Enable Developer Options and USB Debugging
3. In Android Studio: Run > Run 'app'

### Release Build

1. Build > Generate Signed Bundle / APK
2. Select APK
3. Create or select keystore
4. Build release APK
5. Install APK on device

## Testing

### Test WiFi Connection

```kotlin
// Check if connected to POV-POI-WiFi
fun isConnectedToPOVPoiWiFi(context: Context): Boolean {
    val wifiManager = context.applicationContext
        .getSystemService(Context.WIFI_SERVICE) as WifiManager
    val wifiInfo = wifiManager.connectionInfo
    return wifiInfo.ssid.contains("POV-POI-WiFi")
}
```

### Test API Connection

```kotlin
// Test status endpoint
lifecycleScope.launch {
    try {
        val status = api.getStatus()
        Log.d("POVPoi", "Connected: ${status.connected}")
    } catch (e: Exception) {
        Log.e("POVPoi", "Connection failed: ${e.message}")
    }
}
```

## Troubleshooting

**App can't connect:**
- Verify device is connected to POV-POI-WiFi
- Check IP address is 192.168.4.1
- Ensure ESP32 is powered and running
- Check network permissions in manifest

**Slow response:**
- WiFi signal strength
- Serial communication bottleneck
- Reduce update frequency

**Patterns not changing:**
- Check API call syntax
- Verify mode is set correctly
- Monitor serial output on ESP32

## Advanced Features

### Auto-reconnect

```kotlin
class POVPoiAPI {
    private val handler = Handler(Looper.getMainLooper())
    private var reconnectRunnable: Runnable? = null
    
    fun startAutoReconnect(interval: Long = 5000) {
        reconnectRunnable = object : Runnable {
            override fun run() {
                checkConnection()
                handler.postDelayed(this, interval)
            }
        }
        handler.post(reconnectRunnable!!)
    }
    
    fun stopAutoReconnect() {
        reconnectRunnable?.let { handler.removeCallbacks(it) }
    }
}
```

### Pattern Presets

```kotlin
object PatternPresets {
    val RAINBOW = PatternConfig(0, 0, Color(255,0,0), Color(0,0,255), 50)
    val FIRE = PatternConfig(0, 1, Color(255,50,0), Color(255,0,0), 80)
    val OCEAN = PatternConfig(0, 1, Color(0,100,255), Color(0,200,255), 30)
    val SPARKLE = PatternConfig(0, 3, Color(200,0,255), Color(100,0,200), 60)
}
```

### Image Upload

```kotlin
suspend fun uploadImage(imageUri: Uri): Boolean {
    return withContext(Dispatchers.IO) {
        val inputStream = context.contentResolver.openInputStream(imageUri)
        val bytes = inputStream?.readBytes()
        
        val request = Request.Builder()
            .url("$baseUrl/api/image")
            .post(MultipartBody.Builder()
                .setType(MultipartBody.FORM)
                .addFormDataPart(
                    "image", "image.png",
                    RequestBody.create(MediaType.parse("image/*"), bytes!!)
                )
                .build())
            .build()
        
        val response = client.newCall(request).execute()
        response.isSuccessful
    }
}
```

## UI Improvements

- Add SeekBars for brightness/frame rate
- Color picker for pattern colors
- Canvas for live drawing
- Pattern preview animations
- Connection status indicator
- WiFi connection helper

## Further Development

- Save favorite patterns
- Pattern sharing via QR code
- Music synchronization
- Multiple poi control
- Sequence builder
- Image gallery
- Gyroscope-based control

## Resources

- [Android Developer Docs](https://developer.android.com/)
- [OkHttp Documentation](https://square.github.io/okhttp/)
- [Material Design Guidelines](https://material.io/design)
- [Kotlin Coroutines Guide](https://kotlinlang.org/docs/coroutines-guide.html)

## License

Example code is provided as-is for educational purposes.
