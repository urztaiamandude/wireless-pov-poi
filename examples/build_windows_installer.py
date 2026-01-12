#!/usr/bin/env python3
"""
Automated build script for POV POI Image Converter Windows executable
Handles dependency installation, cleanup, and PyInstaller execution
"""

import os
import sys
import shutil
import subprocess
from pathlib import Path

def print_header(message):
    """Print a formatted header message"""
    print("\n" + "=" * 70)
    print(f"  {message}")
    print("=" * 70 + "\n")

def check_python_version():
    """Check if Python version is compatible"""
    print("Checking Python version...")
    version = sys.version_info
    print(f"Python {version.major}.{version.minor}.{version.micro}")
    
    if version.major < 3 or (version.major == 3 and version.minor < 7):
        print("❌ Error: Python 3.7 or higher is required")
        print("Please upgrade Python and try again")
        return False
    
    print("✓ Python version is compatible\n")
    return True

def check_and_install_pyinstaller():
    """Check if PyInstaller is installed, install if missing"""
    print("Checking for PyInstaller...")
    
    try:
        import PyInstaller
        print(f"✓ PyInstaller {PyInstaller.__version__} is installed\n")
        return True
    except ImportError:
        print("PyInstaller not found. Installing...")
        try:
            subprocess.check_call([
                sys.executable, "-m", "pip", "install", 
                "-r", "installer_requirements.txt"
            ])
            print("✓ PyInstaller installed successfully\n")
            return True
        except subprocess.CalledProcessError as e:
            print(f"❌ Failed to install PyInstaller: {e}")
            print("Please install manually: pip install pyinstaller>=5.0")
            return False

def check_dependencies():
    """Check if required dependencies are installed"""
    print("Checking dependencies...")
    
    missing = []
    
    # Check for Pillow
    try:
        import PIL
        print(f"✓ Pillow is installed")
    except ImportError:
        missing.append("Pillow")
        print("❌ Pillow is not installed")
    
    # Check for tkinter
    try:
        import tkinter
        print(f"✓ tkinter is available")
    except ImportError:
        missing.append("tkinter")
        print("❌ tkinter is not available")
    
    if missing:
        print(f"\n❌ Missing dependencies: {', '.join(missing)}")
        print("Install with: pip install -r requirements.txt")
        return False
    
    print("✓ All dependencies are installed\n")
    return True

def clean_build_artifacts():
    """Remove previous build artifacts"""
    print("Cleaning previous build artifacts...")
    
    dirs_to_remove = ['build', 'dist', '__pycache__']
    files_to_remove = ['*.spec']
    
    for dir_name in dirs_to_remove:
        if os.path.exists(dir_name):
            print(f"  Removing {dir_name}/")
            shutil.rmtree(dir_name)
    
    # Remove .spec files
    for spec_file in Path('.').glob('*.spec'):
        print(f"  Removing {spec_file}")
        spec_file.unlink()
    
    print("✓ Cleanup complete\n")

def build_executable():
    """Run PyInstaller to build the executable"""
    print_header("Building Windows Executable")
    
    # Import configuration
    try:
        import setup
        print(f"Using configuration from setup.py")
        print(f"  Application: {setup.APP_NAME} v{setup.VERSION}")
        print(f"  Output: {setup.EXE_NAME}.exe\n")
        
        # Get PyInstaller arguments from configuration
        pyinstaller_args = setup.get_pyinstaller_args()
        main_script = setup.MAIN_SCRIPT
    except ImportError:
        print("Warning: Could not import setup.py, using default configuration")
        # Fallback to manual configuration
        pyinstaller_args = [
            "--onefile",
            "--windowed",
            "--name=POV_POI_Image_Converter",
            "--add-data=image_converter.py;.",
            "--hidden-import=PIL",
            "--hidden-import=PIL.Image",
            "--hidden-import=PIL.ImageTk",
            "--hidden-import=PIL.ImageEnhance",
            "--hidden-import=PIL._imagingtk",
            "--hidden-import=PIL._tkinter_finder",
            "--exclude-module=matplotlib",
            "--exclude-module=numpy",
            "--exclude-module=scipy",
            "--exclude-module=pandas",
        ]
        main_script = "image_converter_gui.py"
    
    # Build full command
    cmd = [sys.executable, "-m", "PyInstaller"] + pyinstaller_args + [main_script]
    
    print("Running PyInstaller...")
    print(f"Command: {' '.join(cmd)}\n")
    
    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"❌ Build failed with error code {e.returncode}")
        print(f"Error output:\n{e.stderr}")
        return False

def verify_output():
    """Verify that the executable was created successfully"""
    print("Verifying output...")
    
    # Import executable name from setup if available
    try:
        import setup
        exe_name = setup.EXE_NAME
    except (ImportError, AttributeError):
        exe_name = "POV_POI_Image_Converter"
    
    exe_path = Path(f"dist/{exe_name}.exe")
    
    if exe_path.exists():
        size_mb = exe_path.stat().st_size / (1024 * 1024)
        print(f"✓ Executable created successfully!")
        print(f"  Location: {exe_path.absolute()}")
        print(f"  Size: {size_mb:.1f} MB\n")
        return True
    else:
        print(f"❌ Executable not found at: {exe_path}")
        return False

def print_success_message():
    """Print success message with next steps"""
    print_header("Build Successful!")
    
    print("Your Windows executable is ready!\n")
    print("📁 Location: dist/POV_POI_Image_Converter.exe\n")
    print("Next steps:")
    print("  1. Test the executable by double-clicking it")
    print("  2. The first launch may take a few seconds (extraction)")
    print("  3. Distribute the .exe file to Windows users\n")
    print("Distribution notes:")
    print("  • Users do NOT need Python installed")
    print("  • File size: ~15-20 MB (typical)")
    print("  • Windows Defender may show a warning (false positive)")
    print("  • Single file - easy to distribute\n")
    print("For more information, see BUILD_INSTALLER.md")

def main():
    """Main build process"""
    print_header("POV POI Image Converter - Windows Build Script")
    
    # Check Python version
    if not check_python_version():
        return 1
    
    # Check and install PyInstaller
    if not check_and_install_pyinstaller():
        return 1
    
    # Check dependencies
    if not check_dependencies():
        print("\nPlease install dependencies first:")
        print("  pip install -r requirements.txt")
        return 1
    
    # Clean previous builds
    clean_build_artifacts()
    
    # Build executable
    if not build_executable():
        print("\n❌ Build failed. Please check the error messages above.")
        return 1
    
    # Verify output
    if not verify_output():
        return 1
    
    # Print success message
    print_success_message()
    
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\n⚠️  Build cancelled by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
