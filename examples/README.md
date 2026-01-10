# Example Images and Patterns

This directory contains example images and pattern configurations for the POV POI system.

## Image Guidelines

### Size Requirements
- **Width**: 31 pixels (matches LED count) - **automatically converted**
- **Height**: Up to 64 pixels recommended (automatically adjusted)
- **Format**: JPG, PNG, GIF (non-animated)

### Automatic Conversion
The system automatically converts images to POV-compatible format:

- **Web Interface**: Uses JavaScript Canvas API to resize images client-side before upload
- **Android App**: Uses Android Bitmap API to resize images before upload  
- **Python Script**: Use `image_converter.py` to pre-convert images
- **Teensy Processing**: Final image processing and storage handled by Teensy 4.1

Images of any size are automatically resized to 31 pixels wide while maintaining aspect ratio. Heights exceeding 64 pixels are cropped.

### Design Tips
- Use high contrast colors
- Simple shapes work better than complex photos
- Bright colors on dark backgrounds
- Test in low-light conditions
- Consider viewing angle of spinning poi

## Creating POV Images

### Automatic Conversion (Recommended)

The system automatically converts images when you upload them:

1. **Web Interface**: Simply select any image and click upload - automatic client-side conversion
2. **Android App**: Use the `uploadImage(bitmap)` method - automatic conversion before upload
3. **Python Script**: Run `python image_converter.py your_image.jpg` for pre-conversion

### Manual Conversion

If you prefer to prepare images manually:

### Using Image Editor (GIMP, Photoshop, etc.)

1. Create new image: 31 × 64 pixels
2. Use "Nearest Neighbor" interpolation when resizing
3. Draw your design
4. Export as PNG
5. Upload via web interface

### Using Python

```python
from PIL import Image

# Create a new image
img = Image.new('RGB', (31, 64), color='black')
pixels = img.load()

# Example: Vertical gradient
for y in range(64):
    for x in range(31):
        pixels[x, y] = (255, y * 4, 0)

# Save
img.save('gradient.png')
```

## Example Patterns

### Rainbow
```json
{
  "type": 0,
  "color1": {"r": 255, "g": 0, "b": 0},
  "color2": {"r": 0, "g": 0, "b": 255},
  "speed": 50
}
```

### Fire
```json
{
  "type": 1,
  "color1": {"r": 255, "g": 50, "b": 0},
  "color2": {"r": 255, "g": 0, "b": 0},
  "speed": 80
}
```

### Ocean Wave
```json
{
  "type": 1,
  "color1": {"r": 0, "g": 100, "b": 255},
  "color2": {"r": 0, "g": 200, "b": 255},
  "speed": 30
}
```

### Purple Sparkle
```json
{
  "type": 3,
  "color1": {"r": 200, "g": 0, "b": 255},
  "color2": {"r": 100, "g": 0, "b": 200},
  "speed": 60
}
```

## Tips for Best Results

1. **High Contrast**: Use bright colors against dark backgrounds
2. **Simple Shapes**: Complex images don't translate well at 31 pixels
3. **Test Small**: Start with simple patterns before complex images
4. **Color Choice**: Primary colors (R, G, B) are brightest
5. **Brightness**: Adjust system brightness based on environment
6. **Spin Speed**: Consistent spin speed is crucial for POV effect
7. **Frame Rate**: Higher frame rates = smoother appearance
