# POV Display Orientation Guide

## LED Strip Layout Diagrams
- **Diagram**: ASCII representation of the 32 LED strip, showcasing how LED 0 is reserved for level shifting, LED 1 (bottom) displays row 0, and LED 31 (top) displays row 30.

```
    LED 31 (Top)
    +-------------+
    |             |
    |    LED 30   |
    |             |
    |    ...      |
    |             |
    |    LED 1    | <---- Row 0 of the image
    |             |
    +-------------+
    LED 0 (Level Shifting)
```

## Image Data Structure Explanation
Images are converted to **31 pixels tall**, matching the 31 display LEDs (LED 1-LED 31). Each image column is streamed as a time slice while the poi spins. For every column:

- **Row 0 → LED 1** (bottom display LED)
- **Row 30 → LED 31** (top display LED)
- **LED 0 is never part of the image** (reserved for level shifting)

This means the image array is indexed from row 0 at the bottom to row 30 at the top, and the firmware only writes LEDs 1-31.

## POV Spinning Effect Visualization
Each column of pixels is shown in sequence as the poi rotates. Persistence of vision blends those time slices into a full 2D image around the arc of the spin:

```
Time   -> [Col 0]  [Col 1]  [Col 2]  ...  [Col N]
Spin   ->   |        |        |           |
Result -> 2D image appears when the columns are shown evenly in rotation
```

Smooth rotation and consistent frame timing are required so the columns align into a stable image.

## Coordinate System Mapping
**Mapping**: Image row 0 maps to LED 1, continuing up to LED 31 corresponding to image row 30.

| Image Row | LED Index | Physical Location |
|-----------|-----------|-------------------|
| 0 | 1 | Bottom display LED |
| 30 | 31 | Top display LED |

## Test Pattern Verification Instructions
Use simple patterns to confirm orientation before loading complex images:

1. **Numbered ladder**: Create a 31 (height) × N (width) image with row labels (0 at bottom, 30 at top) using an image editor or adapting the Pillow-based generator in `examples/test_vertical_flip.py`. Confirm LED 1 shows row 0.
2. **Vertical gradient**: Bottom pixels red → top pixels blue to confirm vertical direction.
3. **Single-row marker**: A bright line on row 0 should appear at LED 1 only.
4. **Spin test**: Display the same pattern while spinning; the image should not flip vertically.

## Troubleshooting for Common Orientation Issues
- **Upside-down image**: Ensure the converter flips vertically (`Image.FLIP_TOP_BOTTOM`) before upload.
- **Wrong colors**: Verify RGB order and APA102 wiring (data/clock). Confirm FastLED color order.
- **LED 0 showing data**: Ensure loops start at LED 1 and LED 0 is reserved for level shifting.
- **Stretched images**: Confirm height is fixed at 31 pixels and aspect ratio is preserved.
- **Missing rows**: Check image height and ensure LED indices 1-31 are addressed.

## Technical Specifications Table
| Spec | Value |
|------|-------|
| Total LEDs | 32 |
| Display LEDs | 31 (LED 1-LED 31) |
| Level-shift LED | LED 0 |
| Image height | 31 pixels (rows 0-30) |
| Brightness range | 0-255 |
| Frame rate range | 10-120 FPS (firmware limit; validate on hardware) |
| LED type | APA102 |

## Quick Reference Commands
```bash
# Convert image (converter auto-resizes to 31px tall) from repo root
python examples/image_converter.py path/to/image.png

# Run orientation tests
pytest examples/test_vertical_flip.py -v

# Build firmware
pio run -e teensy41
pio run -e esp32
```
