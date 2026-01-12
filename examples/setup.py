#!/usr/bin/env python3
"""
PyInstaller setup configuration for POV POI Image Converter
Configuration module used by build_windows_installer.py
"""

# Application metadata
APP_NAME = "POV POI Image Converter"
VERSION = "1.0.0"
AUTHOR = "POV POI Contributors"
DESCRIPTION = "GUI tool for converting images to POV-compatible format"

# Main script to bundle
MAIN_SCRIPT = "image_converter_gui.py"

# Output executable name (without .exe extension)
EXE_NAME = "POV_POI_Image_Converter"

# PyInstaller configuration
CONFIG = {
    # Basic options
    "onefile": True,              # Bundle as single executable
    "windowed": True,             # Hide console window for GUI
    "name": EXE_NAME,             # Output executable name
    
    # Data files to include
    "add_data": [
        "image_converter.py",     # Converter module (required)
    ],
    
    # Hidden imports (modules not auto-detected)
    "hidden_imports": [
        # PIL/Pillow modules
        "PIL",
        "PIL.Image",
        "PIL.ImageTk", 
        "PIL.ImageEnhance",
        "PIL._imagingtk",
        "PIL._tkinter_finder",
        # Tkinter modules
        "tkinter",
        "tkinter.filedialog",
        "tkinter.messagebox",
        "tkinter.ttk",
    ],
    
    # Modules to exclude (reduce size)
    "exclude_modules": [
        "matplotlib",
        "numpy",
        "scipy",
        "pandas",
        "PyQt5",
        "PyQt6",
        "PySide2",
        "PySide6",
    ],
    
    # Optional features
    "icon": None,                 # Path to .ico file (None = use default)
    "version_info": None,         # Path to version info file (optional)
    "upx": True,                  # Use UPX compression
}

def get_pyinstaller_args():
    """
    Generate PyInstaller command-line arguments from configuration
    Returns list of arguments suitable for subprocess call
    """
    args = []
    
    # Basic flags
    if CONFIG["onefile"]:
        args.append("--onefile")
    
    if CONFIG["windowed"]:
        args.append("--windowed")
    
    # Name
    if CONFIG["name"]:
        args.append(f"--name={CONFIG['name']}")
    
    # Data files
    for data_file in CONFIG["add_data"]:
        # Use platform-specific separator
        import os
        if os.name == 'nt':  # Windows
            args.append(f"--add-data={data_file};.")
        else:  # Unix/Linux/Mac
            args.append(f"--add-data={data_file}:.")
    
    # Hidden imports
    for hidden_import in CONFIG["hidden_imports"]:
        args.append(f"--hidden-import={hidden_import}")
    
    # Exclude modules
    for exclude_module in CONFIG["exclude_modules"]:
        args.append(f"--exclude-module={exclude_module}")
    
    # Optional icon
    if CONFIG["icon"]:
        args.append(f"--icon={CONFIG['icon']}")
    
    # Optional version info
    if CONFIG["version_info"]:
        args.append(f"--version-file={CONFIG['version_info']}")
    
    # UPX compression
    if not CONFIG["upx"]:
        args.append("--noupx")
    
    return args

if __name__ == "__main__":
    # Display configuration when run directly
    print("PyInstaller Configuration for POV POI Image Converter")
    print("=" * 60)
    print(f"\nApplication: {APP_NAME}")
    print(f"Version: {VERSION}")
    print(f"Main Script: {MAIN_SCRIPT}")
    print(f"Output Name: {EXE_NAME}.exe")
    print(f"\nConfiguration:")
    for key, value in CONFIG.items():
        print(f"  {key}: {value}")
    print(f"\nPyInstaller arguments:")
    args = get_pyinstaller_args()
    for arg in args:
        print(f"  {arg}")
    print(f"  {MAIN_SCRIPT}")
