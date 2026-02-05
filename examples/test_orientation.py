#!/usr/bin/env python3
"""
POV Display Orientation Test Pattern Generator

Generates test images to verify LED strip orientation and POV display accuracy.
Creates multiple test patterns optimized for 31-pixel tall displays.

Usage:
    python test_orientation.py              # Generate all test patterns
    python test_orientation.py --pattern bars    # Generate specific pattern
    python test_orientation.py --help       # Show help

Output files:
    test_orientation.png     - Red/green/blue horizontal bars
    test_gradient.png        - Smooth vertical gradient (red to blue)
    test_numbered.png        - Row numbers for precise verification
    test_comprehensive.png   - All tests combined
"""

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("ERROR: This script requires Pillow (PIL)")
    print("Install with: pip install Pillow")
    exit(1)

import sys
import os
import argparse

# Constants
POV_HEIGHT = 31  # Fixed height for POV display (LEDs 1-31)
DEFAULT_WIDTH = 64  # Standard width for test patterns

def create_bar_test(width=DEFAULT_WIDTH, height=POV_HEIGHT):
    """
    Create horizontal color bar test pattern.
    
    Pattern:
    - Rows 0-10:  Red (top of image file)
    - Rows 10-20: Green (middle)
    - Rows 20-30: Blue (bottom of image file)
    
    Expected on LED strip (static, not spinning):
    - LED 31 (top):    Blue
    - LED 16 (middle): Green
    - LED 1 (bottom):  Red
    
    Expected in POV display (spinning):
    - Top:    Red (as in file)
    - Middle: Green
    - Bottom: Blue (as in file)
    """
    print(f"Creating bar test pattern ({width}×{height}px)...")
    img = Image.new('RGB', (width, height), 'black')
    draw = ImageDraw.Draw(img)
    
    # Red bar (rows 0-10)
    draw.rectangle([(0, 0), (width-1, 10)], fill=(255, 0, 0))
    
    # Green bar (rows 10-20)
    draw.rectangle([(0, 10), (width-1, 20)], fill=(0, 255, 0))
    
    # Blue bar (rows 20-30)
    draw.rectangle([(0, 20), (width-1, 30)], fill=(0, 0, 255))
    
    return img

def create_gradient_test(width=DEFAULT_WIDTH, height=POV_HEIGHT):
    """
    Create smooth vertical gradient test pattern.
    
    Transitions from red (top) to blue (bottom) through purple.
    Helps verify smooth color rendering and pixel mapping.
    """
    print(f"Creating gradient test pattern ({width}×{height}px)...")
    img = Image.new('RGB', (width, height), 'black')
    
    for y in range(height):
        # Calculate color for this row
        ratio = y / (height - 1)
        r = int(255 * (1 - ratio))  # Red decreases
        g = 0
        b = int(255 * ratio)  # Blue increases
        
        # Draw horizontal line
        for x in range(width):
            img.putpixel((x, y), (r, g, b))
    
    return img

def create_numbered_test(width=DEFAULT_WIDTH, height=POV_HEIGHT):
    """
    Create test pattern with row numbers.
    
    Shows row numbers at regular intervals to verify exact pixel mapping.
    Useful for debugging specific LED positions.
    """
    print(f"Creating numbered test pattern ({width}×{height}px)...")
    img = Image.new('RGB', (width, height), 'black')
    draw = ImageDraw.Draw(img)
    
    # Try to load a font, fall back to default if not available
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 8)
    except:
        try:
            font = ImageFont.truetype("arial.ttf", 8)
        except:
            font = ImageFont.load_default()
    
    # Draw row numbers every 5 rows
    for y in range(0, height, 5):
        # Color changes with position
        color = (
            255 if y < 10 else 0,
            255 if 10 <= y < 20 else 0,
            255 if y >= 20 else 0
        )
        
        # Draw row number
        text = f"R{y}"
        draw.text((2, y), text, fill=color, font=font)
        
        # Draw marker line on the right side
        draw.line([(width-10, y), (width-1, y)], fill=color, width=2)
    
    return img

def create_comprehensive_test(width=DEFAULT_WIDTH, height=POV_HEIGHT):
    """
    Create comprehensive test combining multiple patterns.
    
    Layout (64px wide):
    - Columns 0-15:   Bar test
    - Columns 16-31:  Gradient test
    - Columns 32-47:  Bar test (repeated)
    - Columns 48-63:  Numbered test
    """
    print(f"Creating comprehensive test pattern ({width}×{height}px)...")
    
    # Create individual patterns
    bars = create_bar_test(16, height)
    gradient = create_gradient_test(16, height)
    numbered = create_numbered_test(16, height)
    
    # Combine into one image
    img = Image.new('RGB', (width, height), 'black')
    img.paste(bars, (0, 0))
    img.paste(gradient, (16, 0))
    img.paste(bars, (32, 0))
    img.paste(numbered, (48, 0))
    
    return img

def save_with_info(img, filename, description):
    """Save image and print information."""
    img.save(filename)
    width, height = img.size
    file_size = os.path.getsize(filename)
    print(f"✓ Saved: {filename}")
    print(f"  Size: {width}×{height}px ({file_size:,} bytes)")
    print(f"  Use: {description}")
    print()

def main():
    parser = argparse.ArgumentParser(
        description='Generate POV display orientation test patterns',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python test_orientation.py                    # Generate all patterns
  python test_orientation.py --pattern bars     # Only bar pattern
  python test_orientation.py --width 128        # Custom width

Test Patterns:
  bars          - Red/green/blue horizontal bars (primary orientation test)
  gradient      - Smooth red-to-blue gradient
  numbered      - Row numbers for precise verification
  comprehensive - All patterns combined
  all           - Generate all patterns (default)
        """
    )
    
    parser.add_argument(
        '--pattern',
        choices=['bars', 'gradient', 'numbered', 'comprehensive', 'all'],
        default='all',
        help='Which pattern(s) to generate'
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
        help=f'Image height in pixels (default: {POV_HEIGHT}, fixed for POV display)'
    )
    
    args = parser.parse_args()
    
    # Validate height
    if args.height != POV_HEIGHT:
        print(f"WARNING: Height {args.height} specified, but POV display requires {POV_HEIGHT}px")
        print(f"         Continuing with {args.height}px, but images may not display correctly.")
        print()
    
    print("=" * 60)
    print("POV Display Orientation Test Pattern Generator")
    print("=" * 60)
    print()
    
    patterns = []
    
    if args.pattern == 'all' or args.pattern == 'bars':
        patterns.append(('bars', create_bar_test, 'test_orientation.png', 
                        'Primary orientation test - verify color positions'))
    
    if args.pattern == 'all' or args.pattern == 'gradient':
        patterns.append(('gradient', create_gradient_test, 'test_gradient.png',
                        'Smooth color transitions - verify pixel accuracy'))
    
    if args.pattern == 'all' or args.pattern == 'numbered':
        patterns.append(('numbered', create_numbered_test, 'test_numbered.png',
                        'Row numbers - verify exact LED mapping'))
    
    if args.pattern == 'all' or args.pattern == 'comprehensive':
        patterns.append(('comprehensive', create_comprehensive_test, 'test_comprehensive.png',
                        'Combined tests - verify all aspects'))
    
    # Generate patterns
    for name, create_func, filename, description in patterns:
        img = create_func(args.width, args.height)
        save_with_info(img, filename, description)
    
    print("=" * 60)
    print("✓ Test patterns generated successfully!")
    print("=" * 60)
    print()
    print("Next steps:")
    print("1. Upload test_orientation.png to your POV poi device")
    print("2. Observe LED strip (stationary, not spinning):")
    print("   - LED 31 (top):    Should show BLUE")
    print("   - LED 16 (middle): Should show GREEN")
    print("   - LED 1 (bottom):  Should show RED")
    print("3. Spin the poi and verify POV display:")
    print("   - Top of image:    Should show RED (as in file)")
    print("   - Middle:          Should show GREEN")
    print("   - Bottom:          Should show BLUE (as in file)")
    print()  
    print("If colors don't match, see docs/ORIENTATION_TROUBLESHOOTING.md")


if __name__ == '__main__':
    main()