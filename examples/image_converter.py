#!/usr/bin/env python3
"""
POV Image Converter
Converts regular images to POV-compatible format (31 pixels wide)
"""

try:
    from PIL import Image, ImageEnhance
except ImportError:
    print("This script requires Pillow (PIL)")
    print("Install with: pip install Pillow")
    exit(1)

import sys
import os

def convert_image_for_pov(input_path, output_path=None, width=31, max_height=64, enhance_contrast=True):
    """
    Convert an image to POV-compatible format
    
    Args:
        input_path: Path to input image
        output_path: Path to save converted image (optional)
        width: Target width in pixels (default 31 for POV system)
        max_height: Maximum height in pixels (default 64)
        enhance_contrast: Whether to enhance contrast (default True)
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
        
        # Calculate new height maintaining aspect ratio
        aspect_ratio = img.height / img.width
        new_height = int(width * aspect_ratio)
        
        # Limit height
        if new_height > max_height:
            new_height = max_height
            print(f"Height limited to {max_height} pixels")
        
        # Resize with nearest neighbor for crisp pixels
        print(f"Resizing to {width}x{new_height}")
        img = img.resize((width, new_height), Image.NEAREST)
        
        # Flip vertically so bottom of image is at LED 1 (closest to board)
        # and top of image is at LED 31 (farthest from board)
        print("Flipping image vertically for correct POV orientation")
        img = img.transpose(Image.FLIP_TOP_BOTTOM)
        
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
    print("  - Resize image to 31 pixels wide")
    print("  - Maintain aspect ratio (max 64 pixels high)")
    print("  - Enhance contrast for better visibility")
    print("  - Save as PNG format")
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
