# Android App Example for POV POI Control

This is a complete Android app for controlling the wireless POV POI system with advanced image conversion capabilities.

## Overview

This example demonstrates how to build an Android app that communicates with the POV POI system using the REST API. The app includes a dedicated Image Converter Activity for converting photos to POV-compatible format.

## Features

### Main Controller
- Connect to POV POI WiFi
- Display system status
- Control display modes
- Adjust brightness and frame rate
- Send pattern configurations
- Live drawing mode

### Image Converter (NEW!)
- **Gallery picker** - Select images from your photo library
- **Camera support** - Take new photos directly
- **Real-time preview** - See before and after conversion
- **Adjustable settings** - Configure width, height, and contrast
- **Save to gallery** - Save converted images to device
- **Direct upload** - Send images to POV device
- **Contrast enhancement** - Improve visibility for POV display

## Prerequisites

- Android Studio
- Android SDK (API 21+)
- Kotlin support
- Device with WiFi capability
- Camera (optional, for photo capture)

## Project Structure

```
android_app/
├── MainActivity.kt              # Main control interface
├── ImageConverterActivity.kt    # Image converter (NEW!)
├── POVPoiAPI.kt                 # API client with enhanced conversion
├── AndroidManifest.xml          # Permissions and activities
├── build.gradle                 # Dependencies
├── activity_image_converter.xml # Image converter layout (NEW!)
└── res/
    └── xml/
        └── file_paths.xml       # FileProvider configuration
```

## Setup Instructions

### 1. Create New Android Project

1. Open Android Studio
2. Create New Project
3. Select "Empty Activity"
4. Language: Kotlin
5. Minimum SDK: API 21 (Android 5.0)

### 2. Add Dependencies

See the included `build.gradle` file for complete dependencies including:

```gradle
dependencies {
    // AndroidX Core
    implementation 'androidx.core:core-ktx:1.12.0'
    implementation 'androidx.appcompat:appcompat:1.6.1'
    implementation 'com.google.android.material:material:1.11.0'
    
    // Lifecycle components
    implementation 'androidx.lifecycle:lifecycle-viewmodel-ktx:2.7.0'
    implementation 'androidx.lifecycle:lifecycle-runtime-ktx:2.7.0'
    
    // Coroutines
    implementation 'org.jetbrains.kotlinx:kotlinx-coroutines-android:1.7.3'
    
    // Networking
    implementation 'com.squareup.okhttp3:okhttp:4.12.0'
    
    // Activity Result APIs
    implementation 'androidx.activity:activity-ktx:1.8.2'
}
```

### 3. Add Permissions

See the included `AndroidManifest.xml` file. Key permissions include:

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android">
    
    <!-- Network permissions -->
    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.ACCESS_WIFI_STATE" />
    
    <!-- Storage permissions -->
    <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"
        android:maxSdkVersion="28" />
    
    <!-- Camera permission -->
    <uses-permission android:name="android.permission.CAMERA" />
    
    <!-- For Android 13+ -->
    <uses-permission android:name="android.permission.READ_MEDIA_IMAGES" />
    
    <application ...>
        <activity android:name=".MainActivity" ... />
        <activity android:name=".ImageConverterActivity" ... />
        
        <!-- FileProvider for camera photos -->
        <provider
            android:name="androidx.core.content.FileProvider"
            android:authorities="${applicationId}.provider"
            ... />
    </application>
</manifest>
```

## Using the Image Converter

### Opening the Image Converter

From MainActivity:
```kotlin
binding.imageConverterButton.setOnClickListener {
    startActivity(Intent(this, ImageConverterActivity::class.java))
}
```

### Image Conversion Features

1. **Select Source**
   - Gallery: Choose existing photos
   - Camera: Take new photos

2. **Adjust Settings**
   - Width: 10-100 pixels (default: 31px)
   - Max Height: 10-128 pixels (default: 64px)
   - Contrast Enhancement: Toggle on/off

3. **Convert & Preview**
   - See original and converted side-by-side
   - Converted image scaled up 10x for visibility

4. **Actions**
   - **Save**: Save to device gallery
   - **Upload**: Send directly to POV device
   - **Share**: Share with other apps

### Conversion Algorithm

The app uses nearest-neighbor interpolation for crisp pixels:

```kotlin
fun convertBitmapToPOVFormat(
    bitmap: Bitmap,
    targetWidth: Int = 31,
    maxHeight: Int = 64,
    enhanceContrast: Boolean = true
): Bitmap {
    // Calculate dimensions maintaining aspect ratio
    val aspectRatio = bitmap.height.toFloat() / bitmap.width.toFloat()
    var targetHeight = (targetWidth * aspectRatio).toInt()
    if (targetHeight > maxHeight) targetHeight = maxHeight
    
    // Resize with nearest neighbor
    val resized = Bitmap.createScaledBitmap(
        bitmap, targetWidth, targetHeight, false
    )
    
    // Optional contrast enhancement
    return if (enhanceContrast) {
        enhanceContrast(resized, 2.0f)
    } else {
        resized
    }
}
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

### Uploading Images

```kotlin
// Upload image from bitmap
lifecycleScope.launch {
    val success = api.uploadImage(bitmap)
    if (success) {
        Toast.makeText(this, "Upload successful!", Toast.LENGTH_SHORT).show()
    }
}
```

### Converting Images

```kotlin
// Convert with custom settings
val convertedBitmap = api.convertBitmapToPOVFormatEnhanced(
    bitmap = originalBitmap,
    targetWidth = 31,
    maxHeight = 64,
    enhanceContrast = true
)
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

### App Issues

**App can't connect:**
- Verify device is connected to POV-POI-WiFi
- Check IP address is 192.168.4.1
- Ensure ESP32 is powered and running
- Check network permissions in manifest

**Image Converter issues:**
- **Camera not working**: Check camera permission granted
- **Can't save images**: Check storage permission (Android 9 and below)
- **Upload fails**: Verify WiFi connection to POV POI device

**Slow response:**
- WiFi signal strength
- Serial communication bottleneck
- Reduce update frequency

**Patterns not changing:**
- Check API call syntax
- Verify mode is set correctly
- Monitor serial output on ESP32

### Permissions

The app requires runtime permissions on Android 6.0+:
- Camera (for taking photos)
- Storage (Android 9 and below, for saving images)
- Media Images (Android 13+, for gallery access)

Handle permission requests:
```kotlin
// Request camera permission
if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
    != PackageManager.PERMISSION_GRANTED) {
    ActivityCompat.requestPermissions(
        this,
        arrayOf(Manifest.permission.CAMERA),
        CAMERA_PERMISSION_REQUEST
    )
}
```
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
