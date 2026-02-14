#!/usr/bin/env python3
"""
Test script to verify image conversion maintains correct orientation.

NOTE: No vertical flip is needed because the LED arrangement maps directly:
- LED 1 (bottom of strip) displays bottom of image
- LED 32 (top of strip) displays top of image

This test verifies that images maintain their original top-to-bottom orientation.
"""

import os
import sys
import tempfile
from PIL import Image, ImageDraw

# Import the image converter module
try:
    from image_converter import convert_image_for_pov
except ImportError:
    print("Error: Could not import image_converter module")
    sys.exit(1)

def create_gradient_image(width, height, filename):
    """Create a vertical gradient image (black at top, white at bottom)"""
    img = Image.new('RGB', (width, height))
    draw = ImageDraw.Draw(img)
    
    for y in range(height):
        # Calculate grayscale value (0 at top, 255 at bottom)
        gray = int(255 * y / (height - 1))
        color = (gray, gray, gray)
        draw.line([(0, y), (width - 1, y)], fill=color)
    
    img.save(filename)
    return filename

def test_vertical_flip():
    """Test that image orientation is maintained (no flip needed)"""
    print("Testing image orientation...")
    print("=" * 60)
    print("NOTE: No vertical flip is applied - LEDs map directly to image pixels")
    print("  LED 1 (bottom) = image bottom, LED 32 (top) = image top")
    print()
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create a gradient test image (black at top, white at bottom)
        test_img = os.path.join(tmpdir, "gradient_test.png")
        create_gradient_image(100, 100, test_img)
        print(f"Created gradient test image: black at top, white at bottom")
        
        output_img = os.path.join(tmpdir, "gradient_output.png")
        
        # Convert image (no flip is applied)
        success = convert_image_for_pov(test_img, output_img, 
                                       height=32, max_width=200,
                                       enhance_contrast=False)
        
        if not success:
            print("❌ FAILED: Conversion returned False")
            return False
        
        # Load the converted image
        result = Image.open(output_img)
        
        # Check pixel values
        # Original: top pixel should be black (0,0,0), bottom pixel white (255,255,255)
        # Since no flip is applied, orientation should be preserved
        
        top_pixel = result.getpixel((result.width // 2, 0))  # Middle x, top y
        bottom_pixel = result.getpixel((result.width // 2, result.height - 1))  # Middle x, bottom y
        
        print(f"Result image size: {result.width}x{result.height}")
        print(f"Top pixel (y=0) RGB: {top_pixel}")
        print(f"Bottom pixel (y={result.height-1}) RGB: {bottom_pixel}")
        
        top_brightness = sum(top_pixel) / 3
        bottom_brightness = sum(bottom_pixel) / 3
        
        print(f"Top brightness: {top_brightness:.1f}")
        print(f"Bottom brightness: {bottom_brightness:.1f}")
        
        # Check if orientation is preserved (no flip)
        # Original: top=dark, bottom=bright
        # After conversion (no flip): top should still be dark, bottom still bright
        if bottom_brightness > top_brightness + 50:  # Allow some margin
            print("✓ PASSED: Image orientation correctly preserved!")
            print("  - Top of image (y=0) is dark (as in original)")
            print("  - Bottom of image is bright (as in original)")
            print("  - LED 1 (bottom) will show bright, LED 32 (top) will show dark")
            return True
        else:
            print("❌ FAILED: Image orientation may not be correct")
            print(f"  Expected bottom pixel to be much brighter than top")
            print(f"  Top: {top_brightness:.1f}, Bottom: {bottom_brightness:.1f}")
            return False

if __name__ == "__main__":
    success = test_vertical_flip()
    sys.exit(0 if success else 1)
