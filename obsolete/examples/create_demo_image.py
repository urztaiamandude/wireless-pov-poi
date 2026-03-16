#!/usr/bin/env python3
"""
Create a demonstration image that shows the orientation fix
Creates an arrow pointing up, which after conversion should appear correctly
"""

import os
from PIL import Image, ImageDraw, ImageFont

def create_demo_image(filename):
    """Create a demo image with an upward arrow and text"""
    width, height = 200, 400
    img = Image.new('RGB', (width, height), color='black')
    draw = ImageDraw.Draw(img)
    
    # Draw "TOP" text at the top
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 40)
    except:
        font = ImageFont.load_default()
    draw.text((width//2, 40), "TOP", fill='red', anchor='mt', font=font)
    
    # Draw upward arrow in the middle
    arrow_center_x = width // 2
    arrow_top_y = 120
    arrow_bottom_y = 300
    arrow_width = 60
    
    # Arrow shaft
    draw.rectangle([
        arrow_center_x - 15, arrow_top_y + 40,
        arrow_center_x + 15, arrow_bottom_y
    ], fill='green')
    
    # Arrow head (pointing up)
    draw.polygon([
        (arrow_center_x, arrow_top_y),
        (arrow_center_x - arrow_width//2, arrow_top_y + 40),
        (arrow_center_x + arrow_width//2, arrow_top_y + 40)
    ], fill='green')
    
    # Draw "BOTTOM" text at the bottom
    draw.text((width//2, height - 40), "BOTTOM", fill='blue', anchor='mb', font=font)
    
    img.save(filename)
    print(f"Created demo image: {filename}")
    return filename

if __name__ == "__main__":
    import sys
    output_file = sys.argv[1] if len(sys.argv) > 1 else "demo_arrow.png"
    create_demo_image(output_file)
    
    print("\nThis image has:")
    print("  - 'TOP' text at the top (red)")
    print("  - An upward arrow in the middle (green)")
    print("  - 'BOTTOM' text at the bottom (blue)")
    print("\nAfter conversion with vertical flip:")
    print("  - LED 1 (closest to board) will show 'BOTTOM'")
    print("  - LED 31 (top of 32-LED strip) will show 'TOP'")
    print("  - When held vertically and moved, the arrow will point up correctly!")
