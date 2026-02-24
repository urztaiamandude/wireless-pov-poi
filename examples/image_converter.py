#!/usr/bin/env python3
"""
POV Image Converter
Converts regular images to POV-compatible format.

IMPORTANT: The LED strip forms the VERTICAL axis of the display.
- HEIGHT is FIXED at 32 pixels (matching 32 display LEDs)
- WIDTH is scaled proportionally using the same scale factor as height
- LED 0 (bottom of strip) = bottom of image
- LED 31 (top of strip) = top of image
- LEDs display vertical columns from left to right
- No vertical flip is needed - the LED arrangement maps directly to image pixels
- Hardware level shifter is used (all 32 LEDs are display LEDs)
"""

try:
    from PIL import Image, ImageEnhance
except ImportError:
    print("This script requires Pillow (PIL)")
    print("Install with: pip install Pillow")
    exit(1)

import sys
import os
import struct

# POV binary file format constants (matches Teensy SD storage)
POV_MAGIC = 0x504F5631  # "POV1" in hex
POV_VERSION = 1
SD_MAX_DIMENSION = 1024  # Max width/height allowed by SD storage validation

# The number of display LEDs determines the fixed HEIGHT of POV images
POV_HEIGHT = 32  # All 32 LEDs are used for display (hardware level shifter)

def convert_image_for_pov(input_path, output_path=None, height=32, max_width=200, 
                         enhance_contrast=True, flip_horizontal=False):
    """
    Convert an image to POV-compatible format
    
    The LED strip forms the VERTICAL axis, so:
    - HEIGHT is FIXED at 32 pixels (one pixel per display LED)
    - WIDTH is scaled proportionally using the same scale factor as height
    - LEDs display vertical columns from left to right
    
    Args:
        input_path: Path to input image
        output_path: Path to save converted image (optional)
        height: Target height in pixels (default 32, matching display LEDs)
        max_width: Maximum width in pixels (default 200)
        enhance_contrast: Whether to enhance contrast (default True)
        flip_horizontal: Flip image horizontally (default False)
    
    Note: No vertical flip is applied because the LED arrangement already
    maps correctly: LED 0 (bottom) displays bottom of image, LED 31 (top)
    displays top of image.
    """
    
    if not os.path.exists(input_path):
        print(f"Error: Input file not found: {input_path}")
        return False
    
    try:
        # Load image
        print(f"Loading image: {input_path}")
        img = Image.open(input_path)
        print(f"Original size: {img.width}x{img.height}")
        
        # Convert to RGB if necessary
        if img.mode != 'RGB':
            print(f"Converting from {img.mode} to RGB")
            img = img.convert('RGB')
        
        # Calculate scale factor based on height change
        # Apply the same scale factor to width to prevent warping
        scale_factor = height / img.height
        new_width = int(img.width * scale_factor)
        
        # Limit width
        if new_width > max_width:
            new_width = max_width
            print(f"Width limited to {max_width} pixels")
        
        if new_width < 1:
            new_width = 1
        
        print(f"Scale factor: {scale_factor:.4f} ({height}/{img.height})")
        
        # Resize with nearest neighbor for crisp pixels
        print(f"Resizing to {new_width}x{height}")
        img = img.resize((new_width, height), Image.NEAREST)
        
        # Apply horizontal flip if requested
        if flip_horizontal:
            print("Flipping image horizontally")
            img = img.transpose(Image.FLIP_LEFT_RIGHT)
        
        # No vertical flip needed - LED 0 is bottom, LED 31 is top
        # This maps directly to image coordinates (y=0 is top, but LED 0 shows row 0)
        
        # Enhance contrast if requested
        if enhance_contrast:
            print("Enhancing contrast")
            enhancer = ImageEnhance.Contrast(img)
            img = enhancer.enhance(2.0)
        
        # Generate output path if not provided
        if output_path is None:
            base, ext = os.path.splitext(input_path)
            output_path = f"{base}_pov.png"
        
        # Save as PNG
        print(f"Saving to: {output_path}")
        img.save(output_path, 'PNG')
        
        print("Conversion successful!")
        print(f"Final size: {img.width}x{img.height}")
        return True
        
    except Exception as e:
        print(f"Error during conversion: {e}")
        return False


def convert_image_to_pov_data(input_path, height=32, max_width=400, enhance_contrast=True,
                              flip_horizontal=False, flip_vertical=False):
    """
    Convert an image to POV-compatible RGB data (width, height, bytes).
    Returns (width, height, rgb_bytes) or None on error.
    """
    if not os.path.exists(input_path):
        return None
    try:
        img = Image.open(input_path)
        if img.mode != 'RGB':
            img = img.convert('RGB')
        scale_factor = height / img.height
        new_width = int(img.width * scale_factor)
        if new_width > max_width:
            new_width = max_width
        if new_width < 1:
            new_width = 1
        img = img.resize((new_width, height), Image.NEAREST)
        if flip_horizontal:
            img = img.transpose(Image.FLIP_LEFT_RIGHT)
        if flip_vertical:
            img = img.transpose(Image.FLIP_TOP_BOTTOM)
        if enhance_contrast:
            enhancer = ImageEnhance.Contrast(img)
            img = enhancer.enhance(2.0)
        return (img.width, img.height, img.tobytes())
    except Exception:
        return None


def save_as_pov(input_path, output_path=None, height=32, max_width=400,
                enhance_contrast=True, flip_horizontal=False, flip_vertical=False):
    """
    Convert an image to .pov format for direct SD card upload.
    Output format matches Teensy SD storage (header + RGB data).
    """
    result = convert_image_to_pov_data(
        input_path, height=height, max_width=max_width,
        enhance_contrast=enhance_contrast, flip_horizontal=flip_horizontal,
        flip_vertical=flip_vertical
    )
    if result is None:
        return False
    width, height, rgb_data = result
    if output_path is None:
        base, _ = os.path.splitext(input_path)
        output_path = f"{base}.pov"
    if not output_path.lower().endswith('.pov'):
        output_path += '.pov'
    try:
        data_size = width * height * 3
        header = struct.pack('<IIHHII',
            POV_MAGIC, POV_VERSION, width, height, data_size, 0)
        with open(output_path, 'wb') as f:
            f.write(header)
            f.write(rgb_data)
        print(f"Saved .pov file: {output_path} ({width}x{height})")
        return True
    except Exception as e:
        print(f"Error writing .pov file: {e}")
        return False


def print_usage():
    """Print usage information"""
    print("POV Image Converter")
    print("=" * 50)
    print()
    print("Usage:")
    print("  python image_converter.py <input_image> [output_file]")
    print("  python image_converter.py --pov <input_image> [output.pov]")
    print("  python image_converter.py --pov --batch <input_dir> <output_dir>")
    print()
    print("Options:")
    print("  --pov         Output .pov format for SD card (default: PNG for web upload)")
    print("  --batch       Batch convert all images in directory")
    print("  --height N    Target height in pixels (default: 32)")
    print("  --max-width N Max width in pixels (default: 400, max 1024 for .pov)")
    print()
    print("Examples:")
    print("  python image_converter.py photo.jpg")
    print("  python image_converter.py --pov photo.jpg")
    print("  python image_converter.py --pov photo.jpg images/heart.pov")
    print("  python image_converter.py --pov --batch ./photos ./sd_images")
    print()
    print("For .pov output: Copy files to SD card /images/ folder, then load via web UI.")
    print()
    print("GUI: Run pov_sd_converter.py for a visual interface.")

def main():
    """Main function"""
    args = sys.argv[1:]
    if not args or len(args) < 1:
        print_usage()
        sys.exit(1)

    pov_mode = '--pov' in args
    batch_mode = '--batch' in args
    for opt in ['--pov', '--batch']:
        if opt in args:
            args.remove(opt)

    height = 32
    max_width = 400
    i = 0
    while i < len(args):
        if args[i] == '--height' and i + 1 < len(args):
            height = int(args[i + 1])
            args.pop(i)
            args.pop(i)
            continue
        if args[i] == '--max-width' and i + 1 < len(args):
            max_width = min(int(args[i + 1]), SD_MAX_DIMENSION)
            args.pop(i)
            args.pop(i)
            continue
        i += 1

    if batch_mode:
        if len(args) < 2:
            print("Batch mode requires: <input_dir> <output_dir>")
            sys.exit(1)
        input_dir, output_dir = args[0], args[1]
        if not os.path.isdir(input_dir):
            print(f"Error: Input directory not found: {input_dir}")
            sys.exit(1)
        os.makedirs(output_dir, exist_ok=True)
        exts = ('.jpg', '.jpeg', '.png', '.gif', '.bmp')
        count = 0
        for f in os.listdir(input_dir):
            if f.lower().endswith(exts):
                inp = os.path.join(input_dir, f)
                base = os.path.splitext(f)[0]
                out = os.path.join(output_dir, f"{base}.pov")
                if save_as_pov(inp, out, height=height, max_width=max_width):
                    count += 1
        print(f"\nConverted {count} images to {output_dir}")
        print("Copy the .pov files to your SD card /images/ folder.")
        sys.exit(0 if count > 0 else 1)

    input_path = args[0]
    output_path = args[1] if len(args) > 1 else None

    if pov_mode:
        success = save_as_pov(input_path, output_path, height=height, max_width=max_width)
        if success:
            print("\nCopy this .pov file to your SD card /images/ folder.")
            print("Then load it via the web interface (SD Card Storage section).")
    else:
        success = convert_image_for_pov(input_path, output_path, height=height, max_width=max_width)
        if success:
            print("\nYou can now upload this image via the web interface:")
            print("1. Connect to POV-POI-WiFi")
            print("2. Open http://192.168.4.1")
            print("3. Go to 'Upload Image' section")
            print("4. Select your converted image")
            print("5. Click 'Upload & Display'")

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
