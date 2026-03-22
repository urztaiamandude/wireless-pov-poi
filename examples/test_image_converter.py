#!/usr/bin/env python3
"""
Test script for POV Image Converter
Creates test images and validates conversion functionality

NOTE: The POV display uses LEDs as the VERTICAL axis:
- HEIGHT is FIXED at 32 pixels (matching 32 display LEDs)
- WIDTH is calculated to maintain aspect ratio
"""

import os
import sys
from PIL import Image, ImageDraw
import tempfile
import shutil

# Import the image converter module from current directory
try:
    from image_converter import convert_image_for_pov
except ImportError:
    print("Error: Could not import image_converter module")
    print("Make sure image_converter.py is in the same directory")
    sys.exit(1)

def create_test_image(width, height, color, filename):
    """Create a simple test image"""
    img = Image.new('RGB', (width, height), color=color)
    draw = ImageDraw.Draw(img)
    # Draw a simple pattern
    draw.rectangle([width//4, height//4, 3*width//4, 3*height//4], fill=(255, 255, 0))
    img.save(filename)
    return filename

def test_basic_conversion():
    """Test basic image conversion - HEIGHT should be fixed at 32"""
    print("\n=== Test 1: Basic Conversion ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create test image
        test_img = os.path.join(tmpdir, "test_input.png")
        create_test_image(100, 100, (255, 0, 0), test_img)
        
        output_img = os.path.join(tmpdir, "test_output.png")
        
        # Convert image - HEIGHT is fixed at 32
        success = convert_image_for_pov(test_img, output_img, height=32, max_width=200)
        
        if not success:
            print("❌ FAILED: Conversion returned False")
            return False
        
        # Verify output exists
        if not os.path.exists(output_img):
            print("❌ FAILED: Output file not created")
            return False
        
        # Verify dimensions - HEIGHT should be 32 (fixed)
        result = Image.open(output_img)
        if result.height != 32:
            print(f"❌ FAILED: Expected height 32, got {result.height}")
            return False
        
        print(f"✓ PASSED: Output is {result.width}x{result.height}")
        return True

def test_large_image():
    """Test conversion of large image - HEIGHT fixed at 32"""
    print("\n=== Test 2: Large Image Conversion ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create large test image
        test_img = os.path.join(tmpdir, "large_test.png")
        create_test_image(800, 600, (0, 255, 0), test_img)
        
        output_img = os.path.join(tmpdir, "large_output.png")
        
        # Convert image - HEIGHT is fixed at 32
        success = convert_image_for_pov(test_img, output_img)
        
        if not success:
            print("❌ FAILED: Conversion returned False")
            return False
        
        # Verify dimensions - HEIGHT should be 32 (fixed)
        result = Image.open(output_img)
        if result.height != 32:
            print(f"❌ FAILED: Expected height 32, got {result.height}")
            return False
        
        print(f"✓ PASSED: Large image converted to {result.width}x{result.height}")
        return True

def test_aspect_ratio():
    """Test that aspect ratio is maintained (WIDTH calculated from fixed HEIGHT)"""
    print("\n=== Test 3: Aspect Ratio Preservation ===")
    
    # Maximum acceptable aspect ratio difference
    ASPECT_RATIO_TOLERANCE = 0.2
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create wide image (width > height)
        test_img = os.path.join(tmpdir, "wide_test.png")
        create_test_image(200, 100, (0, 0, 255), test_img)
        
        output_img = os.path.join(tmpdir, "wide_output.png")
        
        # Convert image - HEIGHT is fixed, WIDTH is calculated
        success = convert_image_for_pov(test_img, output_img)
        
        if not success:
            print("❌ FAILED: Conversion returned False")
            return False
        
        # Verify aspect ratio
        result = Image.open(output_img)
        original = Image.open(test_img)
        
        original_ratio = original.width / original.height  # Should be 2.0
        result_ratio = result.width / result.height
        
        # Allow small difference due to rounding
        if abs(original_ratio - result_ratio) > ASPECT_RATIO_TOLERANCE:
            print(f"❌ FAILED: Aspect ratio changed significantly")
            print(f"   Original: {original_ratio:.2f}, Result: {result_ratio:.2f}")
            return False
        
        print(f"✓ PASSED: Aspect ratio preserved ({result_ratio:.2f})")
        return True

def test_width_limiting():
    """Test that very wide images have width limited by max_width"""
    print("\n=== Test 4: Width Limiting ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create very wide image (32 height, very wide)
        test_img = os.path.join(tmpdir, "very_wide_test.png")
        create_test_image(1000, 32, (255, 0, 255), test_img)
        
        output_img = os.path.join(tmpdir, "very_wide_output.png")
        
        # Convert image with max_width limit
        success = convert_image_for_pov(test_img, output_img, max_width=100)
        
        if not success:
            print("❌ FAILED: Conversion returned False")
            return False
        
        # Verify width is limited
        result = Image.open(output_img)
        if result.width > 100:
            print(f"❌ FAILED: Width {result.width} exceeds max 100")
            return False
        
        # Height should still be 32 (fixed)
        if result.height != 32:
            print(f"❌ FAILED: Expected height 32, got {result.height}")
            return False
        
        print(f"✓ PASSED: Width limited to {result.width}, height is {result.height}")
        return True

def test_rgb_conversion():
    """Test that images are converted to RGB"""
    print("\n=== Test 5: RGB Mode Conversion ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create grayscale image
        test_img = os.path.join(tmpdir, "gray_test.png")
        img = Image.new('L', (100, 100), color=128)
        img.save(test_img)
        
        output_img = os.path.join(tmpdir, "gray_output.png")
        
        # Convert image
        success = convert_image_for_pov(test_img, output_img)
        
        if not success:
            print("❌ FAILED: Conversion returned False")
            return False
        
        # Verify RGB mode
        result = Image.open(output_img)
        if result.mode != 'RGB':
            print(f"❌ FAILED: Expected RGB mode, got {result.mode}")
            return False
        
        print(f"✓ PASSED: Image converted to RGB mode")
        return True

def test_invalid_file():
    """Test handling of invalid file"""
    print("\n=== Test 6: Invalid File Handling ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # Try to convert non-existent file
        nonexistent = os.path.join(tmpdir, "nonexistent.png")
        output = os.path.join(tmpdir, "output.png")
        success = convert_image_for_pov(nonexistent, output)
    
    if success:
        print("❌ FAILED: Should return False for non-existent file")
        return False
    
    print("✓ PASSED: Invalid file handled correctly")
    return True

def test_different_formats():
    """Test conversion of different image formats"""
    print("\n=== Test 7: Different Format Support ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        formats = ['PNG', 'JPEG']
        all_passed = True
        
        for fmt in formats:
            ext = 'jpg' if fmt == 'JPEG' else fmt.lower()
            test_img = os.path.join(tmpdir, f"test.{ext}")
            output_img = os.path.join(tmpdir, f"output_{ext}.png")
            
            # Create test image in specific format
            img = Image.new('RGB', (100, 100), color=(128, 128, 128))
            img.save(test_img, format=fmt)
            
            # Convert - HEIGHT is fixed at 32
            success = convert_image_for_pov(test_img, output_img)
            
            if not success:
                print(f"❌ FAILED: {fmt} conversion failed")
                all_passed = False
            else:
                result = Image.open(output_img)
                if result.height != 32:
                    print(f"❌ FAILED: {fmt} output has wrong height (expected 32, got {result.height})")
                    all_passed = False
                else:
                    print(f"  ✓ {fmt} format converted successfully")
        
        if all_passed:
            print("✓ PASSED: All formats converted successfully")
        return all_passed

def test_default_output_naming():
    """Test default output filename generation"""
    print("\n=== Test 8: Default Output Naming ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create test image
        test_img = os.path.join(tmpdir, "myimage.jpg")
        create_test_image(100, 100, (50, 100, 150), test_img)
        
        # Convert without specifying output (should create myimage_pov.png)
        success = convert_image_for_pov(test_img)
        
        if not success:
            print("❌ FAILED: Conversion returned False")
            return False
        
        # Check if default output was created
        expected_output = os.path.join(tmpdir, "myimage_pov.png")
        if not os.path.exists(expected_output):
            print(f"❌ FAILED: Expected output {expected_output} not created")
            return False
        
        print("✓ PASSED: Default output naming works correctly")
        
        # Clean up
        os.remove(expected_output)
        return True

def run_all_tests():
    """Run all tests and report results"""
    print("=" * 60)
    print("POV Image Converter Test Suite")
    print("=" * 60)
    
    tests = [
        ("Basic Conversion", test_basic_conversion),
        ("Large Image", test_large_image),
        ("Aspect Ratio", test_aspect_ratio),
        ("Width Limiting", test_width_limiting),
        ("RGB Conversion", test_rgb_conversion),
        ("Invalid File", test_invalid_file),
        ("Format Support", test_different_formats),
        ("Default Naming", test_default_output_naming),
    ]
    
    results = []
    for test_name, test_func in tests:
        try:
            result = test_func()
            results.append((test_name, result))
        except Exception as e:
            print(f"❌ EXCEPTION in {test_name}: {e}")
            results.append((test_name, False))
    
    # Print summary
    print("\n" + "=" * 60)
    print("TEST SUMMARY")
    print("=" * 60)
    
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for test_name, result in results:
        status = "✓ PASS" if result else "❌ FAIL"
        print(f"{status}: {test_name}")
    
    print(f"\nTotal: {passed}/{total} tests passed")
    print("=" * 60)
    
    return passed == total

if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)
