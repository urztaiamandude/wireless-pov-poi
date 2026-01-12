#!/usr/bin/env python3
"""
Test script to verify vertical flip is working correctly
Creates a gradient image (black at top, white at bottom) and verifies flip
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
    """Test that vertical flip is working correctly"""
    print("Testing vertical flip...")
    print("=" * 60)
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create a gradient test image (black at top, white at bottom)
        test_img = os.path.join(tmpdir, "gradient_test.png")
        create_gradient_image(100, 100, test_img)
        print(f"Created gradient test image: black at top, white at bottom")
        
        output_img = os.path.join(tmpdir, "gradient_output.png")
        
        # Convert image with flip
        success = convert_image_for_pov(test_img, output_img, 
                                       width=31, max_height=64, 
                                       enhance_contrast=False)
        
        if not success:
            print("❌ FAILED: Conversion returned False")
            return False
        
        # Load the converted image
        result = Image.open(output_img)
        
        # Check pixel values
        # Original: top pixel should be black (0,0,0), bottom pixel white (255,255,255)
        # After flip: top pixel should be white, bottom pixel should be black
        
        top_pixel = result.getpixel((15, 0))  # Middle x, top y
        bottom_pixel = result.getpixel((15, result.height - 1))  # Middle x, bottom y
        
        print(f"Result image size: {result.width}x{result.height}")
        print(f"Top pixel (y=0) RGB: {top_pixel}")
        print(f"Bottom pixel (y={result.height-1}) RGB: {bottom_pixel}")
        
        # After vertical flip:
        # - Top of result (y=0) should be bright (originally from bottom)
        # - Bottom of result (y=max) should be dark (originally from top)
        
        top_brightness = sum(top_pixel) / 3
        bottom_brightness = sum(bottom_pixel) / 3
        
        print(f"Top brightness: {top_brightness:.1f}")
        print(f"Bottom brightness: {bottom_brightness:.1f}")
        
        # Check if flip worked (top should be brighter than bottom)
        if top_brightness > bottom_brightness + 50:  # Allow some margin for contrast enhancement
            print("✓ PASSED: Image is correctly flipped vertically!")
            print("  - Top of image (y=0) is bright (originally from bottom)")
            print("  - Bottom of image is dark (originally from top)")
            return True
        else:
            print("❌ FAILED: Image flip may not be working correctly")
            print(f"  Expected top pixel to be much brighter than bottom")
            print(f"  Top: {top_brightness:.1f}, Bottom: {bottom_brightness:.1f}")
            return False

if __name__ == "__main__":
    success = test_vertical_flip()
    sys.exit(0 if success else 1)
