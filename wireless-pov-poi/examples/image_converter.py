#!/usr/bin/env python3
"""
POV Image Converter
Converts regular images to POV-compatible format.

IMPORTANT: The LED strip forms the VERTICAL axis of the display.
- HEIGHT is FIXED at 31 pixels (matching 31 display LEDs)
- WIDTH is calculated to maintain aspect ratio
- LED 1 (bottom of strip) = bottom of image
- LED 31 (top of strip) = top of image
- No vertical flip is needed - the LED arrangement maps directly to image pixels
"""

try:
    from PIL import Image, ImageEnhance
except ImportError:
    print("This script requires Pillow (PIL)")
    print("Install with: pip install Pillow")
    exit(1)

import sys
import os

# The number of display LEDs determines the fixed HEIGHT of POV images
POV_HEIGHT = 31  # LEDs 1-31 are used for display (LED 0 is level shifter)

def convert_image_for_pov(input_path, output_path=None, height=31, max_width=200, 
                         enhance_contrast=True, flip_horizontal=False):
    """
    Convert an image to POV-compatible format
    
    The LED strip forms the VERTICAL axis, so:
    - HEIGHT is FIXED at 31 pixels (one pixel per display LED)
    - WIDTH is calculated to maintain aspect ratio
    
    Args:
        input_path: Path to input image
        output_path: Path to save converted image (optional)
        height: Target height in pixels (default 31, matching display LEDs)
        max_width: Maximum width in pixels (default 200)
        enhance_contrast: Whether to enhance contrast (default True)
        flip_horizontal: Flip image horizontally (default False)
    
    Note: No vertical flip is applied because the LED arrangement already
    maps correctly: LED 1 (bottom) displays bottom of image, LED 31 (top)
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
        
        # Calculate new width maintaining aspect ratio
        # HEIGHT is fixed (31 LEDs), WIDTH is calculated
        aspect_ratio = img.width / img.height
        new_width = int(height * aspect_ratio)
        
        # Limit width
        if new_width > max_width:
            new_width = max_width
            print(f"Width limited to {max_width} pixels")
        
        if new_width < 1:
            new_width = 1
        
        # Resize with nearest neighbor for crisp pixels
        print(f"Resizing to {new_width}x{height}")
        img = img.resize((new_width, height), Image.NEAREST)
        
        # Apply horizontal flip if requested
        if flip_horizontal:
            print("Flipping image horizontally")
            img = img.transpose(Image.FLIP_LEFT_RIGHT)
        
        # No vertical flip needed - LED 1 is bottom, LED 31 is top
        # This maps directly to image coordinates (y=0 is top, but LED 1 shows row 0)
        
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

def print_usage():
    """Print usage information"""
    print("POV Image Converter")
    print("=" * 50)
    print()
    print("Usage:")
    print("  python image_converter.py <input_image> [output_image]")
    print()
    print("Examples:")
    print("  python image_converter.py photo.jpg")
    print("  python image_converter.py photo.jpg pov_photo.png")
    print()
    print("The script will:")
    print("  - Resize image to 31 pixels HIGH (matching 31 display LEDs)")
    print("  - Calculate width to maintain aspect ratio")
    print("  - Enhance contrast for better visibility")
    print("  - Save as PNG format")
    print()
    print("Note: The LED strip forms the VERTICAL axis of the display.")
    print("  - HEIGHT is fixed at 31 pixels (one per LED)")
    print("  - WIDTH is calculated based on aspect ratio")
    print()
    print("Alternative: Use the POISonic online converter:")
    print("  https://web.archive.org/web/20210509110926/https://www.orchardelica.com/poisonic/poi_page.html")

def main():
    """Main function"""
    
    if len(sys.argv) < 2:
        print_usage()
        sys.exit(1)
    
    input_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else None
    
    success = convert_image_for_pov(input_path, output_path)
    
    if success:
        print("\nYou can now upload this image via the web interface:")
        print("1. Connect to POV-POI-WiFi")
        print("2. Open http://192.168.4.1")
        print("3. Go to 'Upload Image' section")
        print("4. Select your converted image")
        print("5. Click 'Upload & Display'")
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
