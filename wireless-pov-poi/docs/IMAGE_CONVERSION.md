# Image Conversion Guide

This guide explains how images are automatically converted to POV-compatible format in the Nebula Poi.

## Overview

**IMPORTANT: The LED strip forms the VERTICAL axis of the POV display.**

- **HEIGHT is FIXED at 31 pixels** - This matches the 31 display LEDs (LED 0 is level shifter)
- **WIDTH is CALCULATED** - Based on the original image's aspect ratio
- **LED 1 (bottom)** displays the **bottom** of the image
- **LED 31 (top)** displays the **top** of the image
- **No vertical flip is needed** - The LED arrangement maps directly to image pixels

## Conversion Flow

### 1. Web Interface (Browser)

**Process:**
1. User selects any image file (JPG, PNG, etc.)
2. JavaScript Canvas API resizes image to 31 pixels **HIGH**
3. Width is calculated to maintain aspect ratio
4. Image is converted to raw RGB data
5. RGB data is uploaded to ESP32
6. ESP32 forwards data to Teensy for final processing

**Advantages:**
- No server-side processing needed
- Works with any image size
- Fast conversion using browser's native image handling
- Reduces data transfer (only RGB data sent)

**Code Location:** `esp32_firmware/esp32_firmware.ino` (JavaScript section in HTML)

### 2. Python GUI Converter (Recommended for Desktop)

**Process:**
1. User runs `python image_converter_gui.py`
2. Select image(s) through file browser
3. Preview before/after conversion in real-time
4. Height is fixed at 31 (matching display LEDs)
5. Width is calculated to maintain aspect ratio
6. PIL/Pillow library resizes image with settings
7. Save converted image(s) to chosen location
8. User uploads pre-converted image to device

**Advantages:**
- User-friendly visual interface
- Real-time preview of conversion
- Before/after comparison
- Adjustable max width setting
- Batch conversion support
- Cross-platform (Windows, Mac, Linux)
- Helpful error messages with installation guidance

**Code Location:** `examples/image_converter_gui.py`

### 3. Python Script (Command-Line Pre-conversion)

**Process:**
1. User runs `python image_converter.py input.jpg`
2. PIL/Pillow library loads and resizes image
3. Image resized to 31 pixels **HIGH**
4. Width calculated based on aspect ratio
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

### 4. Teensy Processing (Final Stage)

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
- **Height:** 31 pixels (FIXED - matches 31 display LEDs)
- **Width:** Calculated to maintain aspect ratio (max configurable)
- **Aspect Ratio:** Maintained from source image

### LED Orientation
- **LED 1** (bottom of strip): Displays **bottom** of image
- **LED 31** (top of strip): Displays **top** of image
- **No flip needed:** The LED arrangement maps directly to image coordinates
- **When spinning:** Image scrolls naturally in correct orientation

### Algorithm
- **Method:** Nearest-neighbor interpolation
- **No flip needed:** LEDs map directly to image pixels
- **Reason:** Preserves crisp pixels for LED display
- **Result:** Sharp, clear images on POV display with natural scrolling

### Color Space
- **Format:** RGB (24-bit color)
- **Channels:** Red, Green, Blue (0-255 each)
- **Storage:** 3 bytes per pixel

### Size Limits
- **Height:** Always 31 pixels (fixed)
- **Width:** Variable based on aspect ratio
- **Minimum:** 1×31 pixels = 93 bytes
- **Typical:** 62×31 pixels = 5,766 bytes (2:1 aspect ratio)

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
3. Adjust settings:
   - Height is fixed at 31 (matching display LEDs)
   - Max width can be adjusted
   - Horizontal flip option
   - Contrast enhancement
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
    
    // HEIGHT is fixed at 31 (matching display LEDs)
    // WIDTH is calculated to maintain aspect ratio
    const targetHeight = 31;
    const targetWidth = Math.round(targetHeight * img.width / img.height);
    
    canvas.width = targetWidth;
    canvas.height = targetHeight;
    
    ctx.imageSmoothingEnabled = false; // Nearest neighbor
    ctx.drawImage(img, 0, 0, targetWidth, targetHeight);
    
    // No flip needed - LEDs map directly to image pixels
    const imageData = ctx.getImageData(0, 0, targetWidth, targetHeight);
    
    // Extract RGB data
    return convertToRGB(imageData);
}
```

### Python (Image Converter)

```python
def convert_image_for_pov(input_path, output_path=None, 
                         height=31, max_width=200):
    img = Image.open(input_path)
    
    # HEIGHT is fixed (display LEDs), WIDTH is calculated
    aspect_ratio = img.width / img.height
    new_width = int(height * aspect_ratio)
    if new_width > max_width:
        new_width = max_width
    
    # Resize with nearest neighbor
    img = img.resize((new_width, height), Image.NEAREST)
    
    # No flip needed - LEDs map directly to image pixels
    
    # Save result
    img.save(output_path, 'PNG')
    return True
```

### C++ (Teensy Processing)

```cpp
void receiveImage() {
    uint8_t srcWidth = cmdBuffer[4];
    uint8_t srcHeight = cmdBuffer[5];
    
    // HEIGHT is fixed (DISPLAY_LEDS), WIDTH is calculated
    uint8_t targetHeight = DISPLAY_LEDS;  // 31
    uint8_t targetWidth = (srcWidth * targetHeight) / srcHeight;
    if (targetWidth > MAX_IMAGE_WIDTH) targetWidth = MAX_IMAGE_WIDTH;
    
    // Nearest-neighbor resize
    for (uint8_t ty = 0; ty < targetHeight; ty++) {
        for (uint8_t tx = 0; tx < targetWidth; tx++) {
            uint8_t sx = (tx * srcWidth) / targetWidth;
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
- [x] Mobile-responsive web interface
- [x] PWA support for installable web app
- [x] Touch-optimized controls for mobile

### Planned Features
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
- [ESP32 Firmware](../esp32_firmware/esp32_firmware.ino)
- [Teensy Firmware](../teensy_firmware/teensy_firmware.ino)

---

For questions or issues with image conversion, check the [Troubleshooting Guide](../TROUBLESHOOTING.md) or open an issue on GitHub.
