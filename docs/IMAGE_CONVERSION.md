# Image Conversion Guide

This guide explains how images are automatically converted to POV-compatible format in the Wireless POV POI system.

## Overview

The POV POI system requires images to be 31 pixels wide (matching the LED strip). Images uploaded through any interface are **automatically converted** to this format.

## Conversion Flow

### 1. Web Interface (Browser)

**Process:**
1. User selects any image file (JPG, PNG, etc.)
2. JavaScript Canvas API resizes image to 31 pixels wide
3. Aspect ratio is maintained (max height: 64 pixels)
4. Image is converted to raw RGB data
5. RGB data is uploaded to ESP32
6. ESP32 forwards data to Teensy for final processing

**Advantages:**
- No server-side processing needed
- Works with any image size
- Fast conversion using browser's native image handling
- Reduces data transfer (only RGB data sent)

**Code Location:** `esp32_firmware/esp32_firmware.ino` (JavaScript section in HTML)

### 2. Android App

**Process:**
1. User selects image from gallery or camera
2. Android Bitmap API resizes image to 31 pixels wide
3. Aspect ratio is maintained (max height: 64 pixels)
4. Image is converted to raw RGB byte array
5. RGB data is uploaded to ESP32
6. ESP32 forwards data to Teensy for final processing

**Advantages:**
- Fast conversion using Android's optimized Bitmap class
- Reduces network transfer
- Works with any Android-supported image format
- Efficient memory usage

**Code Location:** `examples/android_app/POVPoiAPI.kt` - `convertBitmapToPOVFormat()`

### 3. Python GUI Converter (Recommended for Desktop)

**Process:**
1. User runs `python image_converter_gui.py`
2. Select image(s) through file browser
3. Preview before/after conversion in real-time
4. Adjust settings (width, height, contrast)
5. PIL/Pillow library resizes image with settings
6. Save converted image(s) to chosen location
7. User uploads pre-converted image to device

**Advantages:**
- User-friendly visual interface
- Real-time preview of conversion
- Before/after comparison
- Adjustable conversion settings
- Batch conversion support
- Cross-platform (Windows, Mac, Linux)
- Helpful error messages with installation guidance

**Code Location:** `examples/image_converter_gui.py`

### 4. Python Script (Command-Line Pre-conversion)

**Process:**
1. User runs `python image_converter.py input.jpg`
2. PIL/Pillow library loads and resizes image
3. Image resized to 31 pixels wide
4. Aspect ratio maintained, height limited to 64 pixels
5. Contrast enhancement applied (optional)
6. Output saved as PNG file
7. User uploads pre-converted image

**Advantages:**
- Command-line automation
- Scriptable for batch processing
- Contrast enhancement for better visibility
- No GUI dependencies
- Fast for single images

**Code Location:** `examples/image_converter.py`

### 5. Teensy Processing (Final Stage)

**Process:**
1. Teensy receives image data from ESP32
2. If image is already 31px high: store directly
3. If image needs conversion: perform nearest-neighbor resize
4. Store in LED display buffer
5. Ready for POV display

**Advantages:**
- Teensy 4.1 runs at 600 MHz (plenty of power)
- Handles any edge cases
- Final validation and storage
- Supports direct serial upload

**Code Location:** `teensy_firmware/teensy_firmware.ino` - `receiveImage()`

## Conversion Specifications

### Target Dimensions
- **Width:** 31 pixels (fixed)
- **Height:** 1-64 pixels (automatically calculated)
- **Aspect Ratio:** Maintained from source image

### LED Orientation
- **LED 1** (closest to board/handle): Displays **bottom** of image
- **LED 31** (farthest from board): Displays **top** of image
- **Reason:** When poi are held vertically (handle down, LEDs up) and moved horizontally, the image scrolls naturally in correct orientation
- **Implementation:** Images are automatically flipped vertically during conversion

### Algorithm
- **Method:** Nearest-neighbor interpolation
- **Vertical Flip:** Applied after resizing for correct POV orientation
- **Reason:** Preserves crisp pixels for LED display
- **Result:** Sharp, clear images on POV display with natural scrolling

### Color Space
- **Format:** RGB (24-bit color)
- **Channels:** Red, Green, Blue (0-255 each)
- **Storage:** 3 bytes per pixel

### Size Limits
- **Minimum:** 31×1 pixels = 93 bytes
- **Maximum:** 31×64 pixels = 5,952 bytes
- **Typical:** 31×32 pixels = 2,976 bytes

## Testing Image Conversion

### Test GUI Converter

Install dependencies and run the GUI:
```bash
cd examples
pip install -r requirements.txt
python3 image_converter_gui.py
```

**GUI Testing Steps:**
1. Click "Select Image" and choose a test image
2. Verify before/after previews display correctly
3. Adjust settings (width, max height, contrast)
4. Check that preview updates in real-time
5. Click "Convert & Save" and verify output file
6. Test "Select Multiple" for batch conversion

### Run Automated Tests

Test the Python converter:
```bash
cd examples
python3 test_image_converter.py
```

Expected output: 8/8 tests passed

### Test Individual Images

**Using GUI (Recommended):**
```bash
cd examples
python3 image_converter_gui.py
```
Then select your image through the interface.

**Using Command-Line:**
```bash
cd examples
python3 image_converter.py ../path/to/your/image.jpg
```

Output: `image_pov.png` (31 pixels wide)

### Test Web Interface

1. Connect to POV-POI-WiFi
2. Open http://192.168.4.1
3. Upload any image (will be auto-converted)
4. Check ESP32 serial output for conversion details

### Test Android App

1. Load any image in the app
2. Call `api.uploadImage(bitmap)`
3. Image is automatically converted and uploaded
4. Check logcat for conversion details

## Image Quality Tips

### For Best Results

1. **High Contrast:** Use bright colors on dark backgrounds
2. **Simple Designs:** Complex photos don't work well at 31 pixels
3. **Bold Shapes:** Large, clear shapes are most visible
4. **Test First:** Try simple patterns before complex images

### Recommended Source Images

- **Minimum Width:** 100+ pixels (for better detail)
- **Aspect Ratio:** Portrait (taller than wide) works best
- **Format:** PNG (for quality) or JPG (for smaller files)
- **Content:** Icons, symbols, simple graphics

### Avoid

- Very complex photographs (too much detail lost)
- Images with fine text (becomes unreadable)
- Low contrast images (hard to see while spinning)
- Very wide images (extreme aspect ratio changes)

## Troubleshooting

### "Image too large" Error
- **Cause:** Image exceeds 64 pixels in height after conversion
- **Solution:** Already handled - height is automatically limited to 64px

### "Upload failed" Error
- **Check:** WiFi connection to POV-POI-WiFi
- **Check:** ESP32 is powered and responsive
- **Check:** Serial connection between ESP32 and Teensy

### Image Looks Distorted
- **Cause:** Aspect ratio of source image very different from display
- **Solution:** Pre-crop image to roughly 1:2 aspect ratio (e.g., 100×200)

### Colors Look Different
- **Cause:** LED color accuracy vs. screen display
- **Cause:** Brightness settings affect color perception
- **Solution:** Adjust brightness and test in low-light environment

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    IMAGE CONVERSION FLOW                     │
└─────────────────────────────────────────────────────────────┘

User Image (Any Size)
    │
    ├─── Via Web Browser ─────┐
    │    • Canvas API          │
    │    • Resize to 31px      │
    │    • Convert to RGB      │
    │                          ↓
    │                    ESP32 Receiver
    │                          │
    ├─── Via Android App ──────┤
    │    • Bitmap API          │
    │    • Resize to 31px      │
    │    • Convert to RGB      │
    │                          ↓
    └─── Via Python Script ────┤
         • PIL/Pillow          │
         • Resize to 31px      │
         • Save as PNG         │
                               ↓
                        Teensy 4.1 Processor
                               │
                         • Receive RGB data
                         • Validate/resize if needed
                         • Store in display buffer
                               │
                               ↓
                        APA102 LED Display
                         31 LEDs × RGB
```

## Code Examples

### JavaScript (Web Interface)

```javascript
async function convertImageToPOVFormat(file) {
    const img = new Image();
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');
    
    // Resize to 31 pixels wide
    canvas.width = 31;
    canvas.height = calculateHeight(img, 31);
    
    ctx.imageSmoothingEnabled = false; // Nearest neighbor
    ctx.drawImage(img, 0, 0, 31, canvas.height);
    
    // Flip vertically for correct POV orientation
    const imageData = ctx.getImageData(0, 0, 31, canvas.height);
    const flipped = flipVertically(imageData);
    
    // Extract RGB data
    return convertToRGB(flipped);
}
```

### Kotlin (Android App)

```kotlin
private fun convertBitmapToPOVFormat(bitmap: Bitmap): ByteArray {
    val targetWidth = 31
    var targetHeight = (targetWidth * bitmap.height / bitmap.width)
    if (targetHeight > 64) targetHeight = 64
    
    // Resize with nearest neighbor
    val resized = Bitmap.createScaledBitmap(
        bitmap, targetWidth, targetHeight, false
    )
    
    // Flip vertically for correct POV orientation
    val matrix = Matrix()
    matrix.preScale(1.0f, -1.0f)
    val flipped = Bitmap.createBitmap(
        resized, 0, 0, targetWidth, targetHeight, matrix, false
    )
    
    // Extract RGB bytes
    val pixels = IntArray(targetWidth * targetHeight)
    flipped.getPixels(pixels, 0, targetWidth, 0, 0, 
                      targetWidth, targetHeight)
    
    return extractRGBBytes(pixels)
}
```

### Python (Image Converter)

```python
def convert_image_for_pov(input_path, output_path=None, 
                         width=31, max_height=64):
    img = Image.open(input_path)
    
    # Calculate new height
    aspect_ratio = img.height / img.width
    new_height = int(width * aspect_ratio)
    if new_height > max_height:
        new_height = max_height
    
    # Resize with nearest neighbor
    img = img.resize((width, new_height), Image.NEAREST)
    
    # Flip vertically for correct POV orientation
    img = img.transpose(Image.FLIP_TOP_BOTTOM)
    
    # Save result
    img.save(output_path, 'PNG')
    return True
```

### C++ (Teensy Processing)

```cpp
void receiveImage() {
    uint8_t srcWidth = cmdBuffer[4];
    uint8_t srcHeight = cmdBuffer[5];
    
    // Calculate target dimensions
    uint8_t targetHeight = (srcHeight * IMAGE_WIDTH) / srcWidth;
    if (targetHeight > 64) targetHeight = 64;
    
    // Nearest-neighbor resize
    for (uint8_t ty = 0; ty < targetHeight; ty++) {
        for (uint8_t tx = 0; tx < IMAGE_WIDTH; tx++) {
            uint8_t sx = (tx * srcWidth) / IMAGE_WIDTH;
            uint8_t sy = (ty * srcHeight) / targetHeight;
            
            // Copy pixel from source to target
            images[0].pixels[tx][ty] = getSourcePixel(sx, sy);
        }
    }
}
```

## Performance Metrics

### Web Interface
- **Conversion Time:** 50-200ms (depends on image size)
- **Upload Time:** 100-500ms (depends on WiFi)
- **Total Time:** < 1 second typically

### Android App
- **Conversion Time:** 10-50ms (optimized native code)
- **Upload Time:** 100-500ms (depends on WiFi)
- **Total Time:** < 1 second typically

### Python Script
- **Conversion Time:** 50-300ms (depends on image size)
- **File I/O:** 10-50ms
- **Total Time:** < 500ms typically

### Teensy Processing
- **Receive Time:** 50-100ms (serial transfer)
- **Resize Time:** 5-20ms (if needed)
- **Storage Time:** < 1ms
- **Total Time:** < 150ms typically

## Future Enhancements

### Completed Features ✓
- [x] Android app with advanced image conversion
- [x] Image preview before upload (Android app)
- [x] Brightness/contrast adjustment (Android app)
- [x] Mobile-responsive web interface
- [x] PWA support for installable web app
- [x] Touch-optimized controls for mobile

### Planned Features
- [ ] iOS app with image conversion
- [ ] Bulk image upload and conversion
- [ ] Image rotation and flip options
- [ ] Color palette optimization
- [ ] Dithering for better grayscale images
- [ ] Animation support (GIF to frame sequence)

### Advanced Options
- Bilinear interpolation (smoother but slower)
- Edge enhancement filters
- Custom aspect ratio handling
- Image compression for storage
- Multiple image slots

## Mobile-Friendly Features

### Android App Image Converter

The Android app now includes a dedicated Image Converter Activity with:

**Features:**
- Gallery and camera image selection
- Real-time before/after preview
- Adjustable conversion settings:
  - Width: 10-100 pixels (default: 31px)
  - Max Height: 10-128 pixels (default: 64px)
  - Contrast enhancement toggle
- Save converted images to device gallery
- Direct upload to POV device
- Enhanced contrast for better POV visibility

**Usage:**
1. Open Image Converter from main menu
2. Select image from gallery or take photo
3. Adjust conversion settings
4. Click "Convert Image"
5. Save or upload the converted image

See [Android App README](../examples/android_app/README.md) for details.

### Mobile Web Interface

The web interface is now fully mobile-responsive with:

**Mobile Features:**
- Touch-optimized controls (44px minimum touch targets)
- Responsive layouts for phones and tablets
- Larger buttons and sliders for touch
- PWA support - install as native app
- Offline functionality with service worker
- Optimized image upload for mobile

**PWA Installation:**
1. Open http://192.168.4.1 in mobile browser
2. Click "Install App" button (if supported)
3. Or use browser's "Add to Home Screen"
4. App works offline for basic controls

**Mobile Optimizations:**
- CSS optimized for small screens
- Touch-friendly sliders (32px thumbs)
- Pattern buttons in 2-column grid
- Larger text for readability
- No pinch-to-zoom needed

## References

- [Image Converter Source](../examples/image_converter.py)
- [Test Suite](../examples/test_image_converter.py)
- [Android App](../examples/android_app/)
- [Android API](../examples/android_app/POVPoiAPI.kt)
- [Android Image Converter](../examples/android_app/ImageConverterActivity.kt)
- [ESP32 Firmware](../esp32_firmware/esp32_firmware.ino)
- [Teensy Firmware](../teensy_firmware/teensy_firmware.ino)

---

For questions or issues with image conversion, check the [Troubleshooting Guide](../TROUBLESHOOTING.md) or open an issue on GitHub.
