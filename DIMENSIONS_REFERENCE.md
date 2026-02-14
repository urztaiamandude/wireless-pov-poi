# POV Poi Dimensions Reference Guide

## Quick Reference

| Dimension | Value | Notes |
|-----------|-------|-------|
| **Height** | **32 pixels** | **FIXED** - Matches 32 display LEDs |
| **Width** | Variable | Calculated proportionally from source image |
| **Max Width** | ~200 pixels | Configurable limit |
| **Format** | Width×Height (W×H) | Standard image dimension notation |

## LED Strip Orientation

```
        TOP (LED 31)
            ↑
            │
            │  32 LEDs
            │  VERTICAL
            │
            ↓
      BOTTOM (LED 0)
    [Teensy Board]
```

### Key Points:
- **LED strip is VERTICAL** (not horizontal)
- **32 LEDs** from bottom to top
- **LED 0** (bottom) = bottom of image
- **LED 31** (top) = top of image
- **All 32 LEDs** are display LEDs (hardware level shifter used)

## Image Dimensions

### Correct Terminology

✅ **CORRECT:**
- "32 pixels **tall**" or "32 pixels **high**"
- "Height is **fixed** at 32 pixels"
- "Width is **variable** and calculated proportionally"
- "Format: Width×Height (W×H)"
- "Example: 64×32 means 64 wide, 32 tall"

❌ **INCORRECT:**
- "32 pixels wide" ← **WRONG!**
- "32×64" without clarifying which is W vs H
- "Height is variable" ← **WRONG!**
- "Width is fixed" ← **WRONG!**

### Dimension Format

Always use **Width×Height (W×H)** format:
- `16×32` = 16 pixels wide, 32 pixels tall
- `64×32` = 64 pixels wide, 32 pixels tall
- `200×32` = 200 pixels wide, 32 pixels tall

## How Images Are Displayed

### POV (Persistence of Vision) Display

```
Poi spinning clockwise (viewed from above):

    Column 1  Column 2  Column 3  ...  Column N
       ↓         ↓         ↓              ↓
    [32 LEDs] [32 LEDs] [32 LEDs]    [32 LEDs]
       ↓         ↓         ↓              ↓
    Displayed as poi spins left to right
```

### Display Process:
1. Poi spins in a circle
2. Each rotation, LEDs display vertical columns sequentially
3. Columns are shown left-to-right as poi moves
4. Human eye perceives complete image due to persistence of vision

### Example: 64×32 Image

```
Original Image (64×32):
- 64 columns (width)
- 32 rows (height)
- Each column = 32 pixels tall

Display:
- Column 1: LEDs show pixels [0,0] to [0,31]
- Column 2: LEDs show pixels [1,0] to [1,31]
- ...
- Column 64: LEDs show pixels [63,0] to [63,31]
```

## Image Scaling Logic

### Proportional Scaling

**Goal:** Resize image so height = 32 pixels, while maintaining proportions

**Formula:**
```
scale_factor = 32 / original_height
new_width = original_width × scale_factor
new_height = 32 (fixed)
```

**Example 1:** Square image (100×100)
```
scale_factor = 32 / 100 = 0.32
new_width = 100 × 0.32 = 32
Result: 32×32 (square preserved)
```

**Example 2:** Wide image (200×100)
```
scale_factor = 32 / 100 = 0.32
new_width = 200 × 0.32 = 64
Result: 64×32 (2:1 ratio preserved)
```

**Example 3:** Tall image (50×200)
```
scale_factor = 32 / 200 = 0.16
new_width = 50 × 0.16 = 8
Result: 8×32 (proportions preserved)
```

### Why This Matters

**Prevents warping:** Both dimensions scaled by same percentage
**Maintains aspect ratio:** Original proportions preserved
**Optimizes display:** Uses all 32 LEDs efficiently

## Code Examples

### Python (PIL/Pillow)

```python
from PIL import Image

# Load image
img = Image.open('photo.jpg')

# Calculate scale factor based on height
target_height = 32
scale_factor = target_height / img.height

# Apply same scale to width
new_width = int(img.width * scale_factor)
new_height = target_height  # Always 32

# Resize
img = img.resize((new_width, new_height), Image.NEAREST)
# Result: new_width×32 image
```

### JavaScript (Canvas API)

```javascript
const targetHeight = 32;
const scaleFactor = targetHeight / img.height;
const targetWidth = Math.round(img.width * scaleFactor);

canvas.width = targetWidth;
canvas.height = targetHeight;
ctx.drawImage(img, 0, 0, targetWidth, targetHeight);
// Result: targetWidth×32 image
```

## Common Mistakes to Avoid

### Mistake 1: Swapping Width and Height

❌ **WRONG:**
```python
new_height = int(width * aspect_ratio)  # Height should be fixed!
new_width = 32  # Width should be variable!
```

✅ **CORRECT:**
```python
new_height = 32  # Height is always 32
new_width = int(width * scale_factor)  # Width is calculated
```

### Mistake 2: Using Aspect Ratio Instead of Scale Factor

❌ **WRONG:**
```python
aspect_ratio = img.width / img.height
new_width = int(32 * aspect_ratio)  # This works but is less clear
```

✅ **CORRECT:**
```python
scale_factor = 32 / img.height
new_width = int(img.width * scale_factor)  # Clearer: same scale for both
```

### Mistake 3: Saying "32 pixels wide"

❌ **WRONG:** "Images must be 32 pixels wide"
✅ **CORRECT:** "Images must be 32 pixels tall (height)"

## Maximum Dimensions

### Practical Limits

| Dimension | Typical | Maximum | Notes |
|-----------|---------|---------|-------|
| Height | 32 pixels | 32 pixels | Fixed, matches LED count |
| Width | 32-100 pixels | ~200 pixels | Limited by memory and display time |
| File Size | 3-10 KB | ~20 KB | Width × 32 × 3 bytes (RGB) |

### Why Width Limits Exist

1. **Memory:** Teensy 4.1 has limited RAM
2. **Display Time:** Wider images take longer to display per rotation
3. **Practical Use:** Most POV images work best at 32-100 pixels wide

## File Format Specifications

### PNG/JPG Input
- Any size accepted
- Automatically resized to W×32
- RGB color space

### POV Binary Format
```
[Width: 1 byte][Height: 1 byte][RGB Data: W×H×3 bytes]
```

Example: 64×32 image
```
[0x40][0x20][RGB data: 6,144 bytes]
 64    32    64×32×3
```

## Testing Your Understanding

### Quiz

1. Q: What is the fixed dimension?
   A: Height (32 pixels)

2. Q: What format is "64×32"?
   A: Width×Height (64 wide, 32 tall)

3. Q: How is width calculated?
   A: width = original_width × (32 / original_height)

4. Q: Why 32 pixels?
   A: Matches the 32 display LEDs on the strip (hardware level shifter used)

5. Q: Are images "32 pixels wide"?
   A: NO! Images are "32 pixels TALL (height)"

## See Also

- [Image Conversion Guide](docs/IMAGE_CONVERSION.md)
- [Image Converter Scripts](examples/)
- [Firmware Architecture](FIRMWARE_ARCHITECTURE.md)

---

**Remember:** Height is FIXED at 32 pixels. Width is VARIABLE and calculated proportionally.
