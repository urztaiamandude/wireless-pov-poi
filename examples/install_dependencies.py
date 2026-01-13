#!/usr/bin/env python3
"""
Dependency installer for Nebula Poi Image Converter
"""

import subprocess
import sys
import os

def install_dependencies():
    """Install required packages"""
    print("Installing dependencies for Nebula Poi Image Converter...")
    print("-" * 60)
    
    # Get the directory where this script is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    requirements_file = os.path.join(script_dir, "requirements.txt")
    
    if not os.path.exists(requirements_file):
        print(f"Error: requirements.txt not found at {requirements_file}")
        sys.exit(1)
    
    try:
        subprocess.check_call([
            sys.executable, "-m", "pip", "install", "-r", requirements_file
        ])
        print("\n" + "=" * 60)
        print("✓ Dependencies installed successfully!")
        print("=" * 60)
        print("\nYou can now run:")
        print("  python image_converter_gui.py")
        print("\nOr the command-line version:")
        print("  python image_converter.py input.jpg")
    except subprocess.CalledProcessError as e:
        print("\n" + "=" * 60)
        print(f"✗ Error installing dependencies: {e}")
        print("=" * 60)
        print("\nTry installing manually:")
        print("  pip install --user Pillow")
        print("\nOr on Mac/Linux:")
        print("  pip3 install --user Pillow")
        sys.exit(1)

if __name__ == "__main__":
    install_dependencies()
