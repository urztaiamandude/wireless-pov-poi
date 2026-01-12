#!/usr/bin/env python3
"""
PyInstaller setup configuration for POV POI Image Converter
Creates a standalone Windows executable with all dependencies bundled
"""

from PyInstaller.utils.hooks import collect_data_files, collect_submodules
import os

# Application metadata
APP_NAME = "POV POI Image Converter"
VERSION = "1.0.0"
AUTHOR = "POV POI Contributors"
DESCRIPTION = "GUI tool for converting images to POV-compatible format"

# Main script to bundle
MAIN_SCRIPT = "image_converter_gui.py"

# PyInstaller configuration
a = Analysis(
    [MAIN_SCRIPT],
    pathex=[],
    binaries=[],
    datas=[
        # Include image_converter.py module
        ('image_converter.py', '.'),
    ],
    hiddenimports=[
        # PIL/Pillow modules
        'PIL',
        'PIL.Image',
        'PIL.ImageTk',
        'PIL.ImageEnhance',
        'PIL._imagingtk',
        'PIL._tkinter_finder',
        # Tkinter modules
        'tkinter',
        'tkinter.filedialog',
        'tkinter.messagebox',
        'tkinter.ttk',
    ],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[
        # Exclude unnecessary modules to reduce size
        'matplotlib',
        'numpy',
        'scipy',
        'pandas',
        'PyQt5',
        'PyQt6',
        'PySide2',
        'PySide6',
    ],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=None,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=None)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='POV_POI_Image_Converter',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,  # Hide console window for GUI application
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    version='file_version_info.txt',  # Optional: add version info
    icon=None,  # Optional: add custom icon file
)

# Application information
app = BUNDLE(
    exe,
    name=f'{APP_NAME}.app',
    icon=None,
    bundle_identifier=None,
)
