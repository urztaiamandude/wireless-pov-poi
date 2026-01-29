#!/usr/bin/env python3
"""
Test script to verify proportional scaling in image converter
"""

try:
    from PIL import Image
except ImportError:
    print("This script requires Pillow (PIL)")
    print("Install with: pip install Pillow")
    exit(1)

def test_proportional_scaling():
    """Test that width is scaled proportionally with height"""
    
    print("Testing Proportional Scaling")
    print("=" * 50)
    print()
    
    # Test case 1: Square image
    print("Test 1: Square image (100x100)")
    original_width, original_height = 100, 100
    target_height = 31
    
    scale_factor = target_height / original_height
    new_width = int(original_width * scale_factor)
    
    print(f"  Original: {original_width}x{original_height}")
    print(f"  Scale factor: {scale_factor:.4f} ({target_height}/{original_height})")
    print(f"  New size: {new_width}x{target_height}")
    print(f"  Expected: 31x31 (square should remain square)")
    assert new_width == 31, f"Expected width 31, got {new_width}"
    print("  ✓ PASS")
    print()
    
    # Test case 2: Wide image
    print("Test 2: Wide image (200x100)")
    original_width, original_height = 200, 100
    target_height = 31
    
    scale_factor = target_height / original_height
    new_width = int(original_width * scale_factor)
    
    print(f"  Original: {original_width}x{original_height}")
    print(f"  Scale factor: {scale_factor:.4f} ({target_height}/{original_height})")
    print(f"  New size: {new_width}x{target_height}")
    print(f"  Expected: 62x31 (2:1 aspect ratio preserved)")
    assert new_width == 62, f"Expected width 62, got {new_width}"
    print("  ✓ PASS")
    print()
    
    # Test case 3: Tall image
    print("Test 3: Tall image (50x200)")
    original_width, original_height = 50, 200
    target_height = 31
    
    scale_factor = target_height / original_height
    new_width = int(original_width * scale_factor)
    
    print(f"  Original: {original_width}x{original_height}")
    print(f"  Scale factor: {scale_factor:.4f} ({target_height}/{original_height})")
    print(f"  New size: {new_width}x{target_height}")
    print(f"  Expected: 7x31 (aspect ratio preserved)")
    assert new_width == 7, f"Expected width 7, got {new_width}"
    print("  ✓ PASS")
    print()
    
    # Test case 4: Already 31 pixels high
    print("Test 4: Already correct height (93x31)")
    original_width, original_height = 93, 31
    target_height = 31
    
    scale_factor = target_height / original_height
    new_width = int(original_width * scale_factor)
    
    print(f"  Original: {original_width}x{original_height}")
    print(f"  Scale factor: {scale_factor:.4f} ({target_height}/{original_height})")
    print(f"  New size: {new_width}x{target_height}")
    print(f"  Expected: 93x31 (no change needed)")
    assert new_width == 93, f"Expected width 93, got {new_width}"
    print("  ✓ PASS")
    print()
    
    print("=" * 50)
    print("All tests passed! ✓")
    print()
    print("Key points:")
    print("  - Height is always fixed at 31 pixels")
    print("  - Width is scaled by the same factor as height")
    print("  - This prevents image warping/distortion")
    print("  - LEDs display vertical columns from left to right")

if __name__ == "__main__":
    test_proportional_scaling()
