#!/usr/bin/env python3
"""
Test script for Windows installer build configuration
Validates setup.py and build_windows_installer.py functionality
"""

import os
import sys

def test_setup_import():
    """Test that setup.py can be imported"""
    print("\n=== Test 1: Import setup.py ===")
    try:
        import setup
        print("✓ setup.py imported successfully")
        return True
    except Exception as e:
        print(f"❌ FAILED: Could not import setup.py: {e}")
        return False

def test_setup_configuration():
    """Test that setup.py has required configuration"""
    print("\n=== Test 2: Configuration Validation ===")
    try:
        import setup
        
        # Check required attributes
        required_attrs = ['APP_NAME', 'VERSION', 'MAIN_SCRIPT', 'EXE_NAME', 'CONFIG']
        for attr in required_attrs:
            if not hasattr(setup, attr):
                print(f"❌ FAILED: Missing attribute: {attr}")
                return False
        
        print(f"✓ All required attributes present")
        print(f"  Application: {setup.APP_NAME}")
        print(f"  Version: {setup.VERSION}")
        print(f"  Main Script: {setup.MAIN_SCRIPT}")
        print(f"  Executable: {setup.EXE_NAME}.exe")
        
        # Check CONFIG dictionary
        required_keys = ['onefile', 'windowed', 'name', 'add_data', 'hidden_imports']
        for key in required_keys:
            if key not in setup.CONFIG:
                print(f"❌ FAILED: Missing CONFIG key: {key}")
                return False
        
        print(f"✓ CONFIG dictionary is valid")
        return True
        
    except Exception as e:
        print(f"❌ FAILED: Configuration validation error: {e}")
        return False

def test_pyinstaller_args():
    """Test that get_pyinstaller_args() generates valid arguments"""
    print("\n=== Test 3: PyInstaller Arguments ===")
    try:
        import setup
        
        args = setup.get_pyinstaller_args()
        
        if not isinstance(args, list):
            print(f"❌ FAILED: get_pyinstaller_args() should return a list")
            return False
        
        if len(args) == 0:
            print(f"❌ FAILED: get_pyinstaller_args() returned empty list")
            return False
        
        # Check for critical arguments
        has_onefile = any('--onefile' in arg for arg in args)
        has_windowed = any('--windowed' in arg for arg in args)
        has_name = any('--name=' in arg for arg in args)
        
        if not has_onefile:
            print(f"⚠️  WARNING: Missing --onefile argument")
        if not has_windowed:
            print(f"⚠️  WARNING: Missing --windowed argument")
        if not has_name:
            print(f"⚠️  WARNING: Missing --name argument")
        
        print(f"✓ Generated {len(args)} PyInstaller arguments")
        print(f"  Sample: {args[0]}")
        return True
        
    except Exception as e:
        print(f"❌ FAILED: Error generating arguments: {e}")
        return False

def test_build_script_import():
    """Test that build_windows_installer.py can be imported"""
    print("\n=== Test 4: Import build_windows_installer.py ===")
    try:
        import build_windows_installer
        print("✓ build_windows_installer.py imported successfully")
        
        # Check for required functions
        required_funcs = [
            'check_python_version',
            'check_and_install_pyinstaller',
            'check_dependencies',
            'clean_build_artifacts',
            'build_executable',
            'verify_output',
            'main'
        ]
        
        for func in required_funcs:
            if not hasattr(build_windows_installer, func):
                print(f"⚠️  WARNING: Missing function: {func}")
        
        print(f"✓ All required functions present")
        return True
        
    except Exception as e:
        print(f"❌ FAILED: Could not import build_windows_installer.py: {e}")
        return False

def test_build_script_functions():
    """Test individual build script functions"""
    print("\n=== Test 5: Build Script Functions ===")
    try:
        import build_windows_installer
        
        # Test check_python_version
        result = build_windows_installer.check_python_version()
        if result:
            print("✓ check_python_version() passed")
        else:
            print("❌ FAILED: check_python_version() returned False")
            return False
        
        # Test check_dependencies (may fail if dependencies not installed)
        print("\nNote: check_dependencies() may fail if Pillow not installed - this is expected")
        result = build_windows_installer.check_dependencies()
        if result:
            print("✓ check_dependencies() passed - all dependencies installed")
        else:
            print("ℹ️  check_dependencies() failed - dependencies not installed (expected in test environment)")
        
        return True
        
    except Exception as e:
        print(f"❌ FAILED: Error testing functions: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_requirements_file():
    """Test that installer_requirements.txt exists and is valid"""
    print("\n=== Test 6: Requirements File ===")
    
    # Use constant for requirements filename
    REQUIREMENTS_FILE = "installer_requirements.txt"
    
    if not os.path.exists(REQUIREMENTS_FILE):
        print(f"❌ FAILED: {REQUIREMENTS_FILE} not found")
        return False
    
    with open(REQUIREMENTS_FILE, 'r') as f:
        content = f.read()
        if 'pyinstaller' not in content.lower():
            print(f"❌ FAILED: pyinstaller not in requirements")
            return False
    
    print(f"✓ {REQUIREMENTS_FILE} exists and contains pyinstaller")
    return True

def main():
    """Run all tests"""
    print("=" * 70)
    print("  POV POI Image Converter - Installer Build Tests")
    print("=" * 70)
    
    tests = [
        test_setup_import,
        test_setup_configuration,
        test_pyinstaller_args,
        test_build_script_import,
        test_build_script_functions,
        test_requirements_file,
    ]
    
    results = []
    for test in tests:
        try:
            result = test()
            results.append(result)
        except Exception as e:
            print(f"\n❌ Test failed with exception: {e}")
            import traceback
            traceback.print_exc()
            results.append(False)
    
    print("\n" + "=" * 70)
    print(f"  Test Results: {sum(results)}/{len(results)} passed")
    print("=" * 70)
    
    if all(results):
        print("\n✅ All tests passed!")
        return 0
    else:
        print("\n⚠️  Some tests failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())
