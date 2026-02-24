# POV Display Orientation Guide

## LED Strip Layout Diagrams
- **Diagram**: ASCII representation of the 32 LED strip, showcasing how all 32 LEDs are used for display with a hardware level shifter. LED 0 (bottom) displays row 0, and LED 31 (top) displays row 31.

```
    LED 31 (Top)
    +-------------+
    |             |
    |    LED 31   | <---- Row 31 of the image
    |             |
    |    ...      |
    |             |
    |    LED 0    | <---- Row 0 of the image
    |             |
    +-------------+
```

## Image Data Structure Explanation
Images are converted to **32 pixels tall**, matching the 32 display LEDs (LED 0-LED 31). Each image column is streamed as a time slice while the poi spins. For every column:

- **Row 0 → LED 0** (bottom display LED)
- **Row 31 → LED 31** (top display LED)

This means the image array is indexed from row 0 at the bottom to row 31 at the top, and the firmware writes all LEDs 0-31.

## POV Spinning Effect Visualization
Each column of pixels is shown in sequence as the poi rotates. Persistence of vision blends those time slices into a full 2D image around the arc of the spin:

```
Time   -> [Col 0]  [Col 1]  [Col 2]  ...  [Col N]
Spin   ->   |        |        |           |
Result -> 2D image appears when the columns are shown evenly in rotation
```

Smooth rotation and consistent frame timing are required so the columns align into a stable image.

## Coordinate System Mapping
**Mapping**: Image row 0 maps to LED 0, continuing up to LED 31 corresponding to image row 31.

| Image Row | LED Index | Physical Location |
|-----------|-----------|-------------------|
| 0 | 0 | Bottom display LED |
| 31 | 31 | Top display LED |

## Test Pattern Verification Instructions
Use simple patterns to confirm orientation before loading complex images:

1. **Numbered ladder**:
   - Create a 32 (height) × N (width) image with row labels (0 at bottom, 31 at top) using an image editor or by adapting the gradient generator in `examples/test_vertical_flip.py` to draw row labels.
   - Confirm LED 0 shows row 0.
2. **Vertical gradient**: Bottom pixels red → top pixels blue to confirm vertical direction.
3. **Single-row marker**: A bright line on row 0 should appear at LED 0 only.
4. **Spin test**: Display the same pattern while spinning; the image should not flip vertically.

## Troubleshooting for Common Orientation Issues
- **Upside-down image**: Ensure the converter flips vertically (`Image.FLIP_TOP_BOTTOM`) before upload.
- **Wrong colors**: Verify RGB order and APA102 wiring (data/clock). Confirm FastLED color order.
- **Level Shift Issues**: Ensure hardware level shifter is correctly wired.
- **Stretched images**: Confirm height is fixed at 32 pixels and aspect ratio is preserved.
- **Missing rows**: Check image height and ensure LED indices 0-31 are addressed.

## Technical Specifications Table
| Spec | Value |
|------|-------|
| Total LEDs | 32 |
| Display LEDs | 32 (LED 0-LED 31) |
| Level-shift LED | Hardware Level Shifter |
| Image height | 32 pixels (rows 0-31) |
| Brightness range | 0-255 |
| Frame rate range | 10-120 FPS (firmware limit; validate on hardware) |
| LED type | APA102 |

Validation note: spin at the target frame rate and confirm the image remains smooth and stable with no visible flicker or tearing; reduce FPS if artifacts appear.

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
