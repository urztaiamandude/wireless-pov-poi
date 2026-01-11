#!/usr/bin/env python3
"""
Test script for GUI image converter
Tests the core functionality without requiring tkinter display
"""

import os
import sys
import tempfile
from PIL import Image, ImageDraw

# Import the conversion function
try:
    from image_converter import convert_image_for_pov
except ImportError:
    print("Error: Could not import image_converter module")
    sys.exit(1)

def create_test_image(width, height, color, filename):
    """Create a simple test image"""
    img = Image.new('RGB', (width, height), color=color)
    draw = ImageDraw.Draw(img)
    # Draw a simple pattern
    draw.rectangle([width//4, height//4, 3*width//4, 3*height//4], fill=(255, 255, 0))
    img.save(filename)
    return filename

def test_gui_conversion_logic():
    """Test the conversion logic that the GUI would use"""
    print("\n=== Testing GUI Conversion Logic ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create test image
        test_img = os.path.join(tmpdir, "test_input.png")
        create_test_image(200, 300, (255, 0, 0), test_img)
        
        output_img = os.path.join(tmpdir, "test_output.png")
        
        # Test with GUI default settings
        success = convert_image_for_pov(
            test_img, 
            output_img, 
            width=31, 
            max_height=64, 
            enhance_contrast=True
        )
        
        if not success:
            print("❌ FAILED: Conversion returned False")
            return False
        
        # Verify output
        result = Image.open(output_img)
        if result.width != 31:
            print(f"❌ FAILED: Expected width 31, got {result.width}")
            return False
        
        if result.height > 64:
            print(f"❌ FAILED: Height {result.height} exceeds max 64")
            return False
        
        print(f"✓ PASSED: GUI conversion logic works correctly ({result.width}x{result.height})")
        return True

def test_batch_conversion():
    """Test batch conversion logic that GUI would use"""
    print("\n=== Testing Batch Conversion Logic ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create multiple test images
        test_files = []
        for i in range(3):
            test_img = os.path.join(tmpdir, f"test_{i}.png")
            create_test_image(150 + i*50, 200 + i*50, (i*80, 100, 200), test_img)
            test_files.append(test_img)
        
        # Convert all images
        output_dir = os.path.join(tmpdir, "output")
        os.makedirs(output_dir)
        
        success_count = 0
        for test_file in test_files:
            filename = os.path.basename(test_file)
            base_name = os.path.splitext(filename)[0]
            output_path = os.path.join(output_dir, f"{base_name}_pov.png")
            
            success = convert_image_for_pov(
                test_file,
                output_path,
                width=31,
                max_height=64,
                enhance_contrast=True
            )
            
            if success:
                success_count += 1
        
        if success_count == len(test_files):
            print(f"✓ PASSED: Batch conversion successful ({success_count}/{len(test_files)} files)")
            return True
        else:
            print(f"❌ FAILED: Only {success_count}/{len(test_files)} files converted")
            return False

def test_different_settings():
    """Test conversion with different settings"""
    print("\n=== Testing Different Settings ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        test_img = os.path.join(tmpdir, "test.png")
        create_test_image(100, 100, (100, 150, 200), test_img)
        
        # Test different width settings
        test_cases = [
            {"width": 31, "max_height": 64, "enhance_contrast": True, "name": "default"},
            {"width": 20, "max_height": 64, "enhance_contrast": False, "name": "custom width"},
            {"width": 31, "max_height": 40, "enhance_contrast": True, "name": "limited height"},
        ]
        
        all_passed = True
        for i, settings in enumerate(test_cases):
            output = os.path.join(tmpdir, f"output_{i}.png")
            success = convert_image_for_pov(
                test_img,
                output,
                width=settings["width"],
                max_height=settings["max_height"],
                enhance_contrast=settings["enhance_contrast"]
            )
            
            if success:
                result = Image.open(output)
                if result.width != settings["width"]:
                    print(f"❌ FAILED: {settings['name']} - wrong width")
                    all_passed = False
                else:
                    print(f"  ✓ {settings['name']} setting works")
            else:
                print(f"❌ FAILED: {settings['name']} conversion failed")
                all_passed = False
        
        if all_passed:
            print("✓ PASSED: All settings tested successfully")
        return all_passed

def test_error_handling():
    """Test error handling for invalid inputs"""
    print("\n=== Testing Error Handling ===")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        # Test non-existent file
        result = convert_image_for_pov(
            os.path.join(tmpdir, "nonexistent.png"),
            os.path.join(tmpdir, "output.png")
        )
        
        if result == False:
            print("  ✓ Non-existent file handled correctly")
        else:
            print("❌ FAILED: Should return False for non-existent file")
            return False
        
        print("✓ PASSED: Error handling works correctly")
        return True

def test_gui_file_check():
    """Test that GUI files exist and are properly formatted"""
    print("\n=== Testing GUI Files ===")
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    files_to_check = {
        "image_converter_gui.py": ["POVImageConverterGUI", "tkinter", "select_image"],
        "install_dependencies.py": ["install_dependencies", "pip", "requirements.txt"],
        "requirements.txt": ["Pillow"],
    }
    
    all_passed = True
    for filename, keywords in files_to_check.items():
        filepath = os.path.join(script_dir, filename)
        if not os.path.exists(filepath):
            print(f"❌ FAILED: {filename} not found")
            all_passed = False
            continue
        
        with open(filepath, 'r') as f:
            content = f.read()
            for keyword in keywords:
                if keyword not in content:
                    print(f"❌ FAILED: {filename} missing keyword: {keyword}")
                    all_passed = False
                    break
            else:
                print(f"  ✓ {filename} exists with expected content")
    
    if all_passed:
        print("✓ PASSED: All GUI files present and properly formatted")
    return all_passed

def run_all_tests():
    """Run all GUI tests"""
    print("=" * 60)
    print("POV Image Converter GUI Test Suite")
    print("=" * 60)
    
    tests = [
        ("GUI Conversion Logic", test_gui_conversion_logic),
        ("Batch Conversion", test_batch_conversion),
        ("Different Settings", test_different_settings),
        ("Error Handling", test_error_handling),
        ("GUI Files", test_gui_file_check),
    ]
    
    results = []
    for test_name, test_func in tests:
        try:
            result = test_func()
            results.append((test_name, result))
        except Exception as e:
            print(f"❌ EXCEPTION in {test_name}: {e}")
            import traceback
            traceback.print_exc()
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
    
    if passed == total:
        print("\n✓ All GUI tests passed!")
        print("\nNote: Actual GUI display cannot be tested in headless environment.")
        print("To test the full GUI, run on a system with display:")
        print("  python image_converter_gui.py")
    
    return passed == total

if __name__ == "__main__":
    success = run_all_tests()
    sys.exit(0 if success else 1)
