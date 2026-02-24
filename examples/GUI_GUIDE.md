# GUI Image Converter - Visual Guide

## Application Layout

```
┌─────────────────────────────────────────────────────────────────┐
│                  Nebula Poi Image Converter                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  [Select Image]  [Select Multiple]    File: example.jpg        │
│                                                                 │
│  ┌───────────────────────┐  ┌───────────────────────┐         │
│  │      Before           │  │       After           │         │
│  │                       │  │                       │         │
│  │   ┌─────────────┐     │  │   ┌───┐              │         │
│  │   │             │     │  │   │   │              │         │
│  │   │  Original   │     │  │   │POV│              │         │
│  │   │   Image     │     │  │   │   │              │         │
│  │   │             │     │  │   └───┘              │         │
│  │   └─────────────┘     │  │                       │         │
│  │                       │  │                       │         │
│  │ Original: 200x300px   │  │ POV Format: 32x46px  │         │
│  └───────────────────────┘  └───────────────────────┘         │
│                                                                 │
│  ┌── Conversion Settings ───────────────────────────────────┐  │
│  │                                                           │  │
│  │  Width:        [64  ▼] pixels (default: 64)             │  │
│  │  Max Height:   [32  ▼] pixels (default: 32)             │  │
│  │  ☑ Enhance Contrast (recommended for better visibility) │  │
│  │  ☑ Lock Aspect Ratio                                    │  │
│  │                                                           │  │
│  │  Flip Options:                                           │  │
│  │  ☐ Flip Vertical                                         │  │
│  │  ☐ Flip Horizontal                                       │  │
│  │                                                           │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                 │
│  [Convert & Save]  [Batch Convert All]                         │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│ Status: Ready. Select an image to begin.                       │
└─────────────────────────────────────────────────────────────────┘
```

## Features

### 1. Image Selection
- **Select Image**: Choose a single image file
- **Select Multiple**: Select multiple images for batch conversion
- Supported formats: JPG, JPEG, PNG, GIF, BMP

### 2. Preview Panels
- **Before Panel**: Shows original image with dimensions
- **After Panel**: Shows converted POV format with dimensions
- Images are scaled up for better visibility (10x zoom)
- Real-time preview updates when settings change

### 3. Conversion Settings
- **Width**: Target width in pixels (1-100, default: 32 for POV system)
- **Max Height**: Maximum height in pixels (1-200, default: 64)
- **Enhance Contrast**: Toggle contrast enhancement for better visibility
- **Lock Aspect Ratio**: When checked, changing width automatically updates height (and vice versa) to maintain original proportions
- **Flip Vertical**: Skip the automatic vertical flip (useful for pre-flipped images)
- **Flip Horizontal**: Mirror the image left-to-right

### 4. Action Buttons
- **Convert & Save**: Convert current image and save to chosen location
- **Batch Convert All**: Convert all selected images to a chosen directory

### 5. Status Bar
- Shows current operation status
- Displays file information
- Shows progress during batch conversion

## Usage Workflow

### Single Image Conversion

1. Launch the application:
   ```bash
   python image_converter_gui.py
   ```

2. Click **"Select Image"** button

3. Choose your image file from the file browser

4. Preview appears in both panels:
   - Left: Original image
   - Right: Converted POV format

5. Adjust settings if needed:
   - Change width (default: 31)
   - Change max height (default: 64)
   - Toggle contrast enhancement

6. Click **"Convert & Save"**

7. Choose output location and filename

8. Done! Image is saved and ready to upload to POV device

### Batch Conversion

1. Click **"Select Multiple"** button

2. Select multiple image files (Ctrl+click or Cmd+click)

3. First image preview appears

4. Click **"Batch Convert All"**

5. Choose output directory

6. Confirm batch conversion

7. Progress bar shows conversion status

8. All images are converted and saved with "_pov.png" suffix

## Error Handling

### Missing Pillow Dependency

If Pillow is not installed, the application shows a helpful error dialog:

```
┌────────────────────────────────────────┐
│ Missing Dependency                     │
├────────────────────────────────────────┤
│                                        │
│ Pillow library is required.           │
│                                        │
│ Install with:                          │
│ pip install Pillow                     │
│                                        │
│ Or:                                    │
│ python -m pip install Pillow           │
│                                        │
│ Or run the setup script:               │
│ python install_dependencies.py         │
│                                        │
│              [OK]                      │
└────────────────────────────────────────┘
```

### Invalid Image Files

- Error dialog shows if image cannot be loaded
- Status bar displays error message
- Application remains responsive

### File System Errors

- Permission errors are caught and displayed
- Write failures show helpful error messages
- Batch conversion continues even if some files fail

## Technical Details

### Image Processing

- **Scaling Algorithm**: Nearest-neighbor interpolation for crisp pixels
- **Aspect Ratio**: Maintained from original image
- **Color Space**: RGB (24-bit)
- **Contrast Enhancement**: 2.0x factor when enabled
- **Preview Scaling**: 10x zoom for small images

### Performance

- **Single Conversion**: < 1 second typically
- **Batch Conversion**: Runs in background thread for responsiveness
- **Memory Efficient**: Images processed one at a time
- **File Size**: Output PNG files are small (typically < 10KB)

### Compatibility

- **Python Version**: 3.6+
- **Operating Systems**: Windows, macOS, Linux
- **GUI Framework**: tkinter (included with Python)
- **Dependencies**: Pillow >= 9.0.0

## Keyboard Shortcuts

Currently, the application uses standard OS file dialog shortcuts:
- **Ctrl+A / Cmd+A**: Select all in file browser
- **Enter**: Confirm selection
- **Escape**: Cancel dialog

## Tips for Best Results

1. **Use High Contrast Images**: Better visibility on LEDs
2. **Simple Designs**: Complex details lost at 31 pixels width
3. **Portrait Orientation**: Works best for POV display
4. **Test Settings**: Try with/without contrast enhancement
5. **Batch Similar Images**: Same settings applied to all

## Troubleshooting

### GUI Not Opening

1. Check tkinter is installed:
   ```bash
   python -c "import tkinter"
   ```

2. On Linux, install tkinter:
   ```bash
   sudo apt-get install python3-tk
   ```

### Preview Not Showing

- Verify image file is not corrupted
- Try a different image format
- Check console for error messages

### Batch Conversion Slow

- Normal for large number of files
- Progress bar shows activity
- Application remains responsive
- Can continue using other features

## Example Session

```
$ cd examples
$ python install_dependencies.py
Installing dependencies for Nebula Poi Image Converter...
✓ Dependencies installed successfully!

$ python image_converter_gui.py
[GUI opens]

[Select Image button clicked]
[Choose photo.jpg from file browser]
[Preview shows: 800x600 → 31x23 conversion]

[Adjust Max Height to 40]
[Preview updates: 800x600 → 31x40 conversion]

[Convert & Save button clicked]
[Save as: photo_pov.png]
✓ Image converted and saved!

[Upload photo_pov.png to POV device via web interface]
```

## Getting Help

If you encounter issues:

1. Check the troubleshooting section in `examples/README.md`
2. Verify dependencies: `pip list | grep -i pillow`
3. Run with verbose output: Check console for error messages
4. Test with command-line version: `python image_converter.py test.jpg`

## Next Steps

After converting your images:

1. Connect to POV-POI-WiFi network
2. Open http://192.168.4.1 in browser
3. Go to "Upload Image" section
4. Select your converted image
5. Click "Upload & Display"
6. Enjoy your POV display!
