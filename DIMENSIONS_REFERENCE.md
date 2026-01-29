# POV Poi Dimensions Reference Guide

## Quick Reference

| Dimension | Value | Notes |
|-----------|-------|-------|
| **Height** | **31 pixels** | **FIXED** - Matches 31 display LEDs |
| **Width** | Variable | Calculated proportionally from source image |
| **Max Width** | ~200 pixels | Configurable limit |
| **Format** | Width×Height (W×H) | Standard image dimension notation |

## LED Strip Orientation

```
        TOP (LED 31)
            ↑
            │
            │  31 LEDs
            │  VERTICAL
            │
            ↓
      BOTTOM (LED 1)
    [Teensy Board]
```

### Key Points:
- **LED strip is VERTICAL** (not horizontal)
- **31 LEDs** from bottom to top
- **LED 1** (bottom) = bottom of image
- **LED 31** (top) = top of image
- **LED 0** is used for level shifting (not display)

## Image Dimensions

### Correct Terminology

✅ **CORRECT:**
- "31 pixels **tall**" or "31 pixels **high**"
- "Height is **fixed** at 31 pixels"
- "Width is **variable** and calculated proportionally"
- "Format: Width×Height (W×H)"
- "Example: 62×31 means 62 wide, 31 tall"

❌ **INCORRECT:**
- "31 pixels wide" ← **WRONG!**
- "31×64" without clarifying which is W vs H
- "Height is variable" ← **WRONG!**
- "Width is fixed" ← **WRONG!**

### Dimension Format

Always use **Width×Height (W×H)** format:
- `15×31` = 15 pixels wide, 31 pixels tall
- `62×31` = 62 pixels wide, 31 pixels tall
- `200×31` = 200 pixels wide, 31 pixels tall

## How Images Are Displayed

### POV (Persistence of Vision) Display

```
Poi spinning clockwise (viewed from above):

    Column 1  Column 2  Column 3  ...  Column N
       ↓         ↓         ↓              ↓
    [31 LEDs] [31 LEDs] [31 LEDs]    [31 LEDs]
       ↓         ↓         ↓              ↓
    Displayed as poi spins left to right
```

### Display Process:
1. Poi spins in a circle
2. Each rotation, LEDs display vertical columns sequentially
3. Columns are shown left-to-right as poi moves
4. Human eye perceives complete image due to persistence of vision

### Example: 62×31 Image

```
Original Image (62×31):
- 62 columns (width)
- 31 rows (height)
- Each column = 31 pixels tall

Display:
- Column 1: LEDs show pixels [0,0] to [0,30]
- Column 2: LEDs show pixels [1,0] to [1,30]
- ...
- Column 62: LEDs show pixels [61,0] to [61,30]
```

## Image Scaling Logic

### Proportional Scaling

**Goal:** Resize image so height = 31 pixels, while maintaining proportions

**Formula:**
```
scale_factor = 31 / original_height
new_width = original_width × scale_factor
new_height = 31 (fixed)
```

**Example 1:** Square image (100×100)
```
scale_factor = 31 / 100 = 0.31
new_width = 100 × 0.31 = 31
Result: 31×31 (square preserved)
```

**Example 2:** Wide image (200×100)
```
scale_factor = 31 / 100 = 0.31
new_width = 200 × 0.31 = 62
Result: 62×31 (2:1 ratio preserved)
```

**Example 3:** Tall image (50×200)
```
scale_factor = 31 / 200 = 0.155
new_width = 50 × 0.155 = 7.75 ≈ 8
Result: 8×31 (proportions preserved)
```

### Why This Matters

**Prevents warping:** Both dimensions scaled by same percentage
**Maintains aspect ratio:** Original proportions preserved
**Optimizes display:** Uses all 31 LEDs efficiently

## Code Examples

### Python (PIL/Pillow)

```python
from PIL import Image

# Load image
img = Image.open('photo.jpg')

# Calculate scale factor based on height
target_height = 31
scale_factor = target_height / img.height

# Apply same scale to width
new_width = int(img.width * scale_factor)
new_height = target_height  # Always 31

# Resize
img = img.resize((new_width, new_height), Image.NEAREST)
# Result: new_width×31 image
```

### JavaScript (Canvas API)

```javascript
const targetHeight = 31;
const scaleFactor = targetHeight / img.height;
const targetWidth = Math.round(img.width * scaleFactor);

canvas.width = targetWidth;
canvas.height = targetHeight;
ctx.drawImage(img, 0, 0, targetWidth, targetHeight);
// Result: targetWidth×31 image
```

### Kotlin (Android)

```kotlin
val targetHeight = 31
val scaleFactor = targetHeight.toFloat() / bitmap.height
val targetWidth = (bitmap.width * scaleFactor).toInt()

val resized = Bitmap.createScaledBitmap(
    bitmap, targetWidth, targetHeight, false
)
// Result: targetWidth×31 image
```

## Common Mistakes to Avoid

### Mistake 1: Swapping Width and Height

❌ **WRONG:**
```python
new_height = int(width * aspect_ratio)  # Height should be fixed!
new_width = 31  # Width should be variable!
```

✅ **CORRECT:**
```python
new_height = 31  # Height is always 31
new_width = int(width * scale_factor)  # Width is calculated
```

### Mistake 2: Using Aspect Ratio Instead of Scale Factor

❌ **WRONG:**
```python
aspect_ratio = img.width / img.height
new_width = int(31 * aspect_ratio)  # This works but is less clear
```

✅ **CORRECT:**
```python
scale_factor = 31 / img.height
new_width = int(img.width * scale_factor)  # Clearer: same scale for both
```

### Mistake 3: Saying "31 pixels wide"

❌ **WRONG:** "Images must be 31 pixels wide"
✅ **CORRECT:** "Images must be 31 pixels tall (height)"

## Maximum Dimensions

### Practical Limits

| Dimension | Typical | Maximum | Notes |
|-----------|---------|---------|-------|
| Height | 31 pixels | 31 pixels | Fixed, matches LED count |
| Width | 31-100 pixels | ~200 pixels | Limited by memory and display time |
| File Size | 3-10 KB | ~20 KB | Width × 31 × 3 bytes (RGB) |

### Why Width Limits Exist

1. **Memory:** Teensy 4.1 has limited RAM
2. **Display Time:** Wider images take longer to display per rotation
3. **Practical Use:** Most POV images work best at 31-100 pixels wide

## File Format Specifications

### PNG/JPG Input
- Any size accepted
- Automatically resized to W×31
- RGB color space

### POV Binary Format
```
[Width: 1 byte][Height: 1 byte][RGB Data: W×H×3 bytes]
```

Example: 62×31 image
```
[0x3E][0x1F][RGB data: 5,766 bytes]
 62    31    62×31×3
```

## Testing Your Understanding

### Quiz

1. Q: What is the fixed dimension?
   A: Height (31 pixels)

2. Q: What format is "62×31"?
   A: Width×Height (62 wide, 31 tall)

3. Q: How is width calculated?
   A: width = original_width × (31 / original_height)

4. Q: Why 31 pixels?
   A: Matches the 31 display LEDs on the strip

5. Q: Are images "31 pixels wide"?
   A: NO! Images are "31 pixels TALL (height)"

## See Also

- [Image Conversion Guide](docs/IMAGE_CONVERSION.md)
- [Image Converter Scripts](examples/)
- [Android App Documentation](examples/android_app/README.md)
- [Firmware Architecture](FIRMWARE_ARCHITECTURE.md)

---

**Remember:** Height is FIXED at 31 pixels. Width is VARIABLE and calculated proportionally.
