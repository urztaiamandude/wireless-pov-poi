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

### 3. Python Script (Pre-conversion)

**Process:**
1. User runs `python image_converter.py input.jpg`
2. PIL/Pillow library loads and resizes image
3. Image resized to 31 pixels wide
4. Aspect ratio maintained, height limited to 64 pixels
5. Contrast enhancement applied (optional)
6. Output saved as PNG file
7. User uploads pre-converted image

**Advantages:**
- Batch processing support
- Visual preview of converted image
- Contrast enhancement for better visibility
- No processing during upload

**Code Location:** `examples/image_converter.py`

### 4. Teensy Processing (Final Stage)

**Process:**
1. Teensy receives image data from ESP32
2. If image is already 31px wide: store directly
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

### Algorithm
- **Method:** Nearest-neighbor interpolation
- **Reason:** Preserves crisp pixels for LED display
- **Result:** Sharp, clear images on POV display

### Color Space
- **Format:** RGB (24-bit color)
- **Channels:** Red, Green, Blue (0-255 each)
- **Storage:** 3 bytes per pixel

### Size Limits
- **Minimum:** 31×1 pixels = 93 bytes
- **Maximum:** 31×64 pixels = 5,952 bytes
- **Typical:** 31×32 pixels = 2,976 bytes

## Testing Image Conversion

### Run Automated Tests

Test the Python converter:
```bash
cd examples
python3 test_image_converter.py
```

Expected output: 8/8 tests passed

### Test Individual Images

Convert a single image:
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
    
    // Extract RGB data
    const imageData = ctx.getImageData(0, 0, 31, canvas.height);
    return convertToRGB(imageData);
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
    
    // Extract RGB bytes
    val pixels = IntArray(targetWidth * targetHeight)
    resized.getPixels(pixels, 0, targetWidth, 0, 0, 
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

### Planned Features
- [ ] iOS app with image conversion
- [ ] Bulk image upload and conversion
- [ ] Image preview before upload
- [ ] Brightness/contrast adjustment in web interface
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

## References

- [Image Converter Source](../examples/image_converter.py)
- [Test Suite](../examples/test_image_converter.py)
- [Android API](../examples/android_app/POVPoiAPI.kt)
- [ESP32 Firmware](../esp32_firmware/esp32_firmware.ino)
- [Teensy Firmware](../teensy_firmware/teensy_firmware.ino)

---

For questions or issues with image conversion, check the [Troubleshooting Guide](../TROUBLESHOOTING.md) or open an issue on GitHub.
