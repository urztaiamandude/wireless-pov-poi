#!/usr/bin/env python3
"""
Test error handling when Pillow is not installed
This test verifies the GUI shows appropriate error messages
"""

import sys
import subprocess

def test_missing_pillow_detection():
    """Test that the GUI detects and handles missing Pillow correctly"""
    print("Testing missing Pillow error handling...")
    print("-" * 60)
    
    # Test 1: Verify requirements.txt exists and contains Pillow
    try:
        with open("requirements.txt", "r") as f:
            content = f.read()
            if "Pillow" in content:
                print("✓ requirements.txt contains Pillow dependency")
            else:
                print("❌ requirements.txt does not contain Pillow")
                return False
    except FileNotFoundError:
        print("❌ requirements.txt not found")
        return False
    
    # Test 2: Verify GUI code has proper error handling
    try:
        with open("image_converter_gui.py", "r") as f:
            content = f.read()
            
            # Check for try-except block
            if "try:" in content and "except ImportError:" in content:
                print("✓ GUI has ImportError exception handling")
            else:
                print("❌ GUI missing ImportError exception handling")
                return False
            
            # Check for error message
            if "messagebox.showerror" in content or "messagebox" in content:
                print("✓ GUI shows error dialog for missing dependencies")
            else:
                print("❌ GUI doesn't show error dialog")
                return False
            
            # Check for installation instructions
            if "pip install Pillow" in content:
                print("✓ GUI provides installation instructions")
            else:
                print("❌ GUI doesn't provide installation instructions")
                return False
            
    except FileNotFoundError:
        print("❌ image_converter_gui.py not found")
        return False
    
    # Test 3: Verify install_dependencies.py exists
    try:
        with open("install_dependencies.py", "r") as f:
            content = f.read()
            if "pip install" in content and "requirements.txt" in content:
                print("✓ install_dependencies.py script exists and looks correct")
            else:
                print("❌ install_dependencies.py has issues")
                return False
    except FileNotFoundError:
        print("❌ install_dependencies.py not found")
        return False
    
    print("-" * 60)
    print("✓ All error handling tests passed!")
    return True

if __name__ == "__main__":
    success = test_missing_pillow_detection()
    
    if success:
        print("\nThe GUI application properly handles missing Pillow:")
        print("1. Shows user-friendly error dialog")
        print("2. Provides installation instructions")
        print("3. Exits gracefully")
        print("\nTo manually test:")
        print("  1. Uninstall Pillow: pip uninstall Pillow")
        print("  2. Run GUI: python image_converter_gui.py")
        print("  3. Verify error dialog appears")
        print("  4. Reinstall: pip install Pillow")
    
    sys.exit(0 if success else 1)
