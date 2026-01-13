# Image Orientation Fix - Visual Demonstration

## Problem Statement

For image conversion, the number of LEDs should represent the height, with the first LED (closest to the board) being the bottom of the picture and the last LED being the top. This way the picture scrolls naturally when the poi are held vertically and moved horizontally.

## Solution

All image conversion tools now automatically flip images vertically during conversion.

## Visual Demonstration

### Before Conversion (Original Image)
![Original Demo Image](demo_arrow.png)

The original image has:
- **TOP** text at the top (red)
- An upward arrow in the middle (green)
- **BOTTOM** text at the bottom (blue)

### After Conversion (POV Format)
![Converted POV Image](demo_arrow_pov.png)

The converted image (31x62 pixels) is vertically flipped:
- What was at the **bottom** is now at y=0 (displayed on LED 1, closest to board)
- What was at the **top** is now at y=61 (displayed on LED 31, farthest from board)

## LED Mapping

```
Nebula Poi Physical Layout:

LED 31 (farthest) ─────┐  ← Shows "TOP" (red text)
LED 30            ─────┤
LED 29            ─────┤
    ...           ─────┤  ← Shows arrow pointing up
LED 3             ─────┤
LED 2             ─────┤
LED 1 (closest)   ─────┘  ← Shows "BOTTOM" (blue text)
                  │
                Board/Handle
```

## Result

When the poi are held vertically (handle down, LEDs up) and moved horizontally:
- The image displays in the correct orientation
- The arrow points upward as expected
- Text reads correctly from bottom to top
- The image "scrolls" naturally across space

## Testing

To test this yourself:

```bash
cd examples
python3 create_demo_image.py my_test.png
python3 image_converter.py my_test.png my_test_pov.png
```

Compare the original and converted images - you'll see the vertical flip has been applied.

## Implementation

The vertical flip is implemented in all conversion tools:

1. **Python**: `img.transpose(Image.FLIP_TOP_BOTTOM)`
2. **Android**: Matrix with `preScale(1.0f, -1.0f)`
3. **Web**: Manual pixel-by-pixel flip in JavaScript Canvas

See [IMAGE_CONVERSION.md](../docs/IMAGE_CONVERSION.md) for complete technical details.
