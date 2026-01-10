# Example Images and Patterns

This directory contains example images and pattern configurations for the POV POI system.

## Image Guidelines

### Size Requirements
- **Width**: 31 pixels (matches LED count)
- **Height**: Up to 64 pixels recommended
- **Format**: JPG, PNG, GIF (non-animated)

### Design Tips
- Use high contrast colors
- Simple shapes work better than complex photos
- Bright colors on dark backgrounds
- Test in low-light conditions
- Consider viewing angle of spinning poi

## Creating POV Images

### Using Online Image Converter

For a quick and easy conversion, you can use the POISonic online image converter:
- **POISonic Image Converter**: [https://web.archive.org/web/20210509110926/https://www.orchardelica.com/poisonic/poi_page.html](https://web.archive.org/web/20210509110926/https://www.orchardelica.com/poisonic/poi_page.html)

This tool can help you convert regular images to POV-compatible formats directly in your browser.

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
