# POV Image Converter Tools

## GUI Image Converter (Recommended for Desktop)

### Installation

1. Install dependencies:
```bash
cd examples
pip install -r requirements.txt
```

Or run the setup script:
```bash
python install_dependencies.py
```

### Usage

```bash
python image_converter_gui.py
```

**Features:**
- Visual interface with before/after preview
- Adjust conversion settings (width, height, contrast)
- Single image or batch conversion
- Cross-platform (Windows, Mac, Linux)

**Steps:**
1. Click "Select Image" to choose an image
2. Preview the conversion in real-time
3. Adjust settings if needed (width: 31px, max height: 64px)
4. Click "Convert & Save" to save the POV-compatible image
5. Upload to your POV device via web interface

## Command-Line Image Converter

For automated/batch processing via command line:

```bash
python image_converter.py input.jpg [output.png]
```

**Examples:**
```bash
# Convert single image
python image_converter.py photo.jpg

# Convert with custom output name
python image_converter.py photo.jpg converted.png
```

## Image Guidelines

### Size Requirements
- **Width**: 31 pixels (matches LED count) - **automatically converted**
- **Height**: Up to 64 pixels recommended (automatically adjusted)
- **Format**: JPG, PNG, GIF (non-animated)

### Automatic Conversion
The system automatically converts images to POV-compatible format:

- **GUI Converter**: Desktop application with visual preview (this tool)
- **Web Interface**: Uses JavaScript Canvas API to resize images client-side before upload
- **Android App**: Uses Android Bitmap API to resize images before upload  
- **Command-Line Script**: Use `image_converter.py` to pre-convert images
- **Teensy Processing**: Final image processing and storage handled by Teensy 4.1

Images of any size are automatically resized to 31 pixels wide while maintaining aspect ratio. Heights exceeding 64 pixels are cropped.

### Design Tips
- Use high contrast colors
- Simple shapes work better than complex photos
- Bright colors on dark backgrounds
- Test in low-light conditions
- Consider viewing angle of spinning poi

## Alternative Conversion Methods

### Using Online Image Converter

For a quick and easy conversion, you can use the POISonic online image converter:
- **POISonic Image Converter**: [https://web.archive.org/web/20210509110926/https://www.orchardelica.com/poisonic/poi_page.html](https://web.archive.org/web/20210509110926/https://www.orchardelica.com/poisonic/poi_page.html)

This tool can help you convert regular images to POV-compatible formats directly in your browser.

### Direct Upload (No Pre-Conversion Needed)

The system automatically converts images when you upload them:

1. **Web Interface**: Simply select any image and click upload - automatic client-side conversion
2. **Android App**: Use the `uploadImage(bitmap)` method - automatic conversion before upload

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

## Troubleshooting

### "No module named 'PIL'" Error

The GUI and command-line converters require the Pillow library. Install it with:

```bash
pip install Pillow
```

If that doesn't work, try:

```bash
python -m pip install Pillow
```

Or use the provided setup script:

```bash
python install_dependencies.py
```

### Permission Errors During Installation

**On Mac/Linux:**
```bash
pip install --user Pillow
```

**On Windows:**
Run Command Prompt as Administrator, then:
```bash
pip install Pillow
```

### GUI Window Not Opening

1. Make sure you have tkinter installed (comes with Python by default)
2. On Linux, you may need to install: `sudo apt-get install python3-tk`
3. Try running from command line to see error messages:
   ```bash
   python image_converter_gui.py
   ```

### Image Preview Not Showing

- Make sure the image file is not corrupted
- Try a different image format (PNG or JPG)
- Check console output for error messages

### Batch Conversion Fails

- Ensure output directory has write permissions
- Check that all selected files are valid image files
- Look for error messages in the status bar
