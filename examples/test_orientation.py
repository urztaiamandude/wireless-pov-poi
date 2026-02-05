#!/usr/bin/env python3
"""
POV Display Orientation Test Pattern Generator

Generates test images to verify LED display orientation for the
Nebula POI wireless POV system.

This script creates multiple test patterns:
1. Red/Green/Blue bars - Primary orientation test
2. Gradient test - Smooth color transition
3. Numbered rows - Precise LED mapping verification
4. Comprehensive test - All patterns combined

Usage:
    python test_orientation.py              # Generate all test patterns
    python test_orientation.py --pattern rgb # Generate specific pattern
    python test_orientation.py --help        # Show help

Expected Results:
    Static LED Strip (not spinning):
    - LED 31 (top): Shows last rows (BLUE in rgb test)
    - LED 1 (bottom): Shows first rows (RED in rgb test)
    - LED 0: Always OFF (level shifter)
    
    POV Display (spinning):
    - Top of image: RED (as designed)
    - Middle: GREEN
    - Bottom: BLUE (as designed)
"""

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("Error: This script requires Pillow (PIL)")
    print("Install with: pip install Pillow")
    exit(1)

import sys
import os
import argparse

# POV Display specifications
POV_HEIGHT = 31  # Fixed height matching display LEDs (1-31)
DEFAULT_WIDTH = 64  # Default width for test patterns

def create_rgb_bars(width=DEFAULT_WIDTH, height=POV_HEIGHT):
    """
    Create red/green/blue bar test pattern.
    
    This is the primary orientation test:
    - Rows 0-10: RED (should appear at top when spinning)
    - Rows 11-20: GREEN (middle)
    - Rows 21-30: BLUE (bottom)
    
    Returns:
        PIL.Image: Test pattern image
    """
    img = Image.new('RGB', (width, height), 'black')
    draw = ImageDraw.Draw(img)
    
    # Red bar (top of image file, rows 0-10)
    draw.rectangle([(0, 0), (width-1, 10)], fill=(255, 0, 0))
    
    # Green bar (middle, rows 11-20)
    draw.rectangle([(0, 11), (width-1, 20)], fill=(0, 255, 0))
    
    # Blue bar (bottom of image file, rows 21-30)
    draw.rectangle([(0, 21), (width-1, 30)], fill=(0, 0, 255))
    
    return img

def create_gradient(width=DEFAULT_WIDTH, height=POV_HEIGHT):
    """
    Create smooth gradient from red to blue.
    
    Useful for verifying smooth color transitions and
    detecting any row swapping or inversion issues.
    
    Returns:
        PIL.Image: Gradient test pattern
    """
    img = Image.new('RGB', (width, height), 'black')
    
    for y in range(height):
        # Interpolate from red (255,0,0) to blue (0,0,255)
        ratio = y / (height - 1)
        r = int(255 * (1 - ratio))
        g = 0
        b = int(255 * ratio)
        
        for x in range(width):
            img.putpixel((x, y), (r, g, b))
    
    return img

def create_numbered_rows(width=DEFAULT_WIDTH, height=POV_HEIGHT):
    """
    Create pattern with row numbers displayed.
    
    Shows the actual row index (0-30) on each row to verify
    precise LED-to-row mapping. Useful for debugging specific
    LED positioning issues.
    
    Returns:
        PIL.Image: Numbered row pattern
    """
    img = Image.new('RGB', (width, height), (20, 20, 20))  # Dark gray background
    draw = ImageDraw.Draw(img)
    
    # Try to use a small font (may not work on all systems)
    try:
        # Attempt to use default font at small size
        font = ImageFont.load_default()
    except:
        font = None
    
    # Draw row numbers
    for y in range(height):
        # Alternate colors for visibility
        if y < 10:
            color = (255, 0, 0)  # Red
        elif y < 20:
            color = (0, 255, 0)  # Green
        else:
            color = (0, 0, 255)  # Blue
        
        # Draw row number text
        text = str(y)
        if font:
            draw.text((2, y), text, fill=color, font=font)
        else:
            # Fallback: draw a line pattern indicating row number
            line_length = min(y + 5, width - 5)
            draw.line([(0, y), (line_length, y)], fill=color, width=1)
    
    return img

def create_comprehensive(width=DEFAULT_WIDTH, height=POV_HEIGHT):
    """
    Create comprehensive test pattern combining multiple tests.
    
    Layout:
    - Left third: RGB bars
    - Middle third: Gradient
    - Right third: Checkerboard pattern
    
    Returns:
        PIL.Image: Comprehensive test pattern
    """
    img = Image.new('RGB', (width, height), 'black')
    
    third_width = width // 3;
    
    # Left section: RGB bars
    for y in range(height):
        if y <= 10:
            color = (255, 0, 0)  # Red
        elif y <= 20:
            color = (0, 255, 0)  # Green
        else:
            color = (0, 0, 255)  # Blue
        
        for x in range(third_width):
            img.putpixel((x, y), color);
    
    # Middle section: Gradient
    for y in range(height):
        ratio = y / (height - 1);
        r = int(255 * (1 - ratio));
        b = int(255 * ratio);
        
        for x in range(third_width, 2 * third_width):
            img.putpixel((x, y), (r, 0, b));
    
    # Right section: Checkerboard
    checker_size = 2;
    for y in range(height):
        for x in range(2 * third_width, width):
            if ((y // checker_size) + (x // checker_size)) % 2 == 0:
                img.putpixel((x, y), (255, 255, 255));  # White
            else:
                img.putpixel((x, y), (0, 0, 0));  # Black
    
    return img

def main():
    """Main function to generate test patterns."""
    parser = argparse.ArgumentParser(
        description='Generate POV display orientation test patterns',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python test_orientation.py                    # Generate all patterns
  python test_orientation.py --pattern rgb      # Only RGB bars
  python test_orientation.py --width 128        # Custom width
  python test_orientation.py --all              # Generate all (explicit)

Test Patterns:
  rgb           - Red/Green/Blue bars (primary test)
  gradient      - Smooth red-to-blue gradient
  numbered      - Row numbers for precise mapping
  comprehensive - Combined test pattern
  all           - Generate all patterns (default)
        """
    )
    
    parser.add_argument(
        '--pattern',
        choices=['rgb', 'gradient', 'numbered', 'comprehensive', 'all'],
        default='all',
        help='Test pattern to generate (default: all)'
    )
    
    parser.add_argument(
        '--width',
        type=int,
        default=DEFAULT_WIDTH,
        help=f'Image width in pixels (default: {DEFAULT_WIDTH})'
    )
    
    parser.add_argument(
        '--height',
        type=int,
        default=POV_HEIGHT,
        help=f'Image height in pixels (default: {POV_HEIGHT}, MUST be 31 for POV display)'
    )
    
    parser.add_argument(
        '--all',
        action='store_true',
        help='Generate all test patterns (default behavior)'
    )
    
    args = parser.parse_args()
    
    # Validate height
    if args.height != POV_HEIGHT:
        print(f"WARNING: Height set to {args.height}, but POV display requires {POV_HEIGHT} pixels!")
        print(f"Images will not display correctly unless height is {POV_HEIGHT}.")
        response = input("Continue anyway? (y/N): ")
        if response.lower() != 'y':
            print("Aborted.")
            return
    
    width = args.width
    height = args.height
    
    # Determine which patterns to generate
    patterns_to_generate = []
    
    if args.pattern == 'all' or args.all:
        patterns_to_generate = ['rgb', 'gradient', 'numbered', 'comprehensive']
    else:
        patterns_to_generate = [args.pattern]
    
    print(f"Generating test patterns ({width}x{height} pixels)...")
    print()    
    # Generate patterns
    generated_files = []
    
    for pattern_name in patterns_to_generate:
        filename = f"test_{pattern_name}.png"
        
        print(f"Creating {pattern_name} pattern...")
        
        if pattern_name == 'rgb':
            img = create_rgb_bars(width, height)
            description = "Red/Green/Blue bars - Primary orientation test"
        elif pattern_name == 'gradient':
            img = create_gradient(width, height)
            description = "Smooth red-to-blue gradient"
        elif pattern_name == 'numbered':
            img = create_numbered_rows(width, height)
            description = "Row numbers for LED mapping verification"
        elif pattern_name == 'comprehensive':
            img = create_comprehensive(width, height)
            description = "Combined test pattern"
        
        img.save(filename)
        generated_files.append((filename, description))
        print(f"  ✓ Saved: {filename}")
    
    print()    
    print("=" * 70)
    print("Test patterns generated successfully!")
    print("=" * 70)
    print()    
    for filename, description in generated_files:
        file_size = os.path.getsize(filename)
        print(f"📄 {filename}")
        print(f"   {description}")
        print(f"   Size: {file_size:,} bytes")
        print()    
    print("Next Steps:")
    print("1. Upload test_rgb.png to your POV display")
    print("2. Check static LED strip (not spinning):")
    print("   - Top of strip (LED 31): Should show BLUE")
    print("   - Middle (LEDs 16-20): Should show GREEN")
    print("   - Bottom (LED 1): Should show RED")
    print("   - LED 0: Always OFF (black)")
    print()    
    print("3. Spin the poi and verify POV display:")
    print("   - Top of perceived image: RED")
    print("   - Middle: GREEN")
    print("   - Bottom: BLUE")
    print()    
    print("If orientation is incorrect, see docs/ORIENTATION_TROUBLESHOOTING.md")
    print()    
if __name__ == '__main__':
    main()