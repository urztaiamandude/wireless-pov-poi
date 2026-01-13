# Building Windows Executable for Nebula Poi Image Converter

This guide explains how to create a standalone Windows executable (.exe) for the Nebula Poi Image Converter GUI, allowing Windows users to run the application without installing Python.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Building the Installer](#building-the-installer)
- [Distribution](#distribution)
- [For End Users](#for-end-users)
- [Optional: Inno Setup Integration](#optional-inno-setup-integration)
- [Troubleshooting](#troubleshooting)
- [Technical Details](#technical-details)

## Prerequisites

### For Building the Executable

- **Windows Operating System** (Windows 7 or later)
- **Python 3.7 or higher** installed
  - Download from [python.org](https://www.python.org/downloads/)
  - Make sure to check "Add Python to PATH" during installation
- **All required dependencies** (Pillow, tkinter)

### Verify Python Installation

Open Command Prompt or PowerShell and run:

```bash
python --version
```

You should see something like `Python 3.9.x` or higher.

## Quick Start

The fastest way to build the executable:

```bash
cd examples
pip install -r installer_requirements.txt
python build_windows_installer.py
```

That's it! The executable will be created in `examples/dist/POV_POI_Image_Converter.exe`

## Building the Installer

### Configuration File

The build process uses `setup.py` as a centralized configuration file that contains:
- Application metadata (name, version)
- PyInstaller options (onefile, windowed, etc.)
- Hidden imports for PIL/Pillow and tkinter
- Modules to exclude for size optimization

You can view the configuration by running:
```bash
python setup.py
```

To modify the build configuration, edit `setup.py` and adjust the `CONFIG` dictionary.

### Step 1: Install Runtime Dependencies

First, install the required libraries for the application:

```bash
cd examples
pip install -r requirements.txt
```

This installs:
- Pillow (image processing library)

### Step 2: Install Build Dependencies

Install PyInstaller, the tool that creates the executable:

```bash
pip install -r installer_requirements.txt
```

This installs:
- PyInstaller 5.0 or higher

### Step 3: Run the Build Script

Execute the automated build script:

```bash
python build_windows_installer.py
```

The script will:
1. ✓ Check Python version compatibility
2. ✓ Verify PyInstaller is installed (install if missing)
3. ✓ Check all dependencies are present
4. ✓ Clean previous build artifacts
5. ✓ Run PyInstaller with proper configuration
6. ✓ Verify the executable was created successfully
7. ✓ Display the output location and file size

### Expected Output

```
======================================================================
  Nebula Poi Image Converter - Windows Build Script
======================================================================

Checking Python version...
Python 3.9.7
✓ Python version is compatible

Checking for PyInstaller...
✓ PyInstaller 5.13.0 is installed

Checking dependencies...
✓ Pillow is installed
✓ tkinter is available
✓ All dependencies are installed

Cleaning previous build artifacts...
✓ Cleanup complete

======================================================================
  Building Windows Executable
======================================================================

Running PyInstaller...
[... build output ...]

Verifying output...
✓ Executable created successfully!
  Location: C:\...\examples\dist\POV_POI_Image_Converter.exe
  Size: 18.3 MB

======================================================================
  Build Successful!
======================================================================

Your Windows executable is ready!

📁 Location: dist/POV_POI_Image_Converter.exe
```

### Manual Build (Alternative)

If you prefer to run PyInstaller manually:

```bash
pyinstaller --onefile --windowed --name=POV_POI_Image_Converter ^
  --add-data="image_converter.py;." ^
  --hidden-import=PIL --hidden-import=PIL.Image ^
  --hidden-import=PIL.ImageTk --hidden-import=PIL.ImageEnhance ^
  image_converter_gui.py
```

## Distribution

### Locating the Executable

After a successful build, you'll find:

```
examples/
├── dist/
│   └── POV_POI_Image_Converter.exe    ← This is your distributable file
├── build/                              ← Temporary files (can be deleted)
└── POV_POI_Image_Converter.spec       ← Build configuration (can be deleted)
```

### File Information

- **File**: `POV_POI_Image_Converter.exe`
- **Location**: `examples/dist/`
- **Size**: ~15-20 MB (typical)
- **Type**: Standalone executable (no installation required)

### Distributing to Users

1. **Simple Method**: Share the `.exe` file directly
   - Upload to cloud storage (Google Drive, Dropbox, etc.)
   - Send via email (if size permits)
   - Host on GitHub Releases page

2. **Professional Method**: Create an installer with Inno Setup (see [Optional: Inno Setup Integration](#optional-inno-setup-integration))

3. **Best Practices**:
   - Provide a README with usage instructions
   - Mention that Windows Defender may show warnings (false positive)
   - Include a link to the project repository
   - Consider code signing for distribution (see [Code Signing](#code-signing))

## For End Users

### Running the Executable

**No Python installation required!** Users can simply:

1. Download `POV_POI_Image_Converter.exe`
2. Double-click the file to run
3. Wait a few seconds for the first launch (extraction)
4. Use the GUI to convert images

### System Requirements

- Windows 7 or later
- ~50 MB free disk space
- No additional software required

### First Launch

The first time you run the executable:
- Windows may show a security warning (see [Troubleshooting](#windows-defender-warnings))
- Extraction takes 5-10 seconds (creates temporary files)
- Subsequent launches are faster

### Usage

Once launched, the application works exactly like the Python version:

1. Click "Select Image" to choose an image file
2. Preview the conversion in real-time
3. Adjust settings (width, height, contrast)
4. Click "Convert & Save" to export the POV-compatible image
5. Upload the converted image to your POV device

## Optional: Inno Setup Integration

For a professional Windows installer with proper installation, shortcuts, and uninstall support:

### Install Inno Setup

Download from: https://jrsoftware.org/isdl.php

### Basic Inno Setup Script

Create `installer_script.iss`:

```iss
[Setup]
AppName=Nebula Poi Image Converter
AppVersion=1.0.0
DefaultDirName={autopf}\Nebula Poi Image Converter
DefaultGroupName=Nebula Poi Image Converter
OutputDir=installer_output
OutputBaseFilename=POV_POI_Image_Converter_Setup
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "dist\POV_POI_Image_Converter.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\POV POI Image Converter"; Filename: "{app}\POV_POI_Image_Converter.exe"
Name: "{group}\Uninstall POV POI Image Converter"; Filename: "{uninstallexe}"
Name: "{autodesktop}\POV POI Image Converter"; Filename: "{app}\POV_POI_Image_Converter.exe"

[Run]
Filename: "{app}\POV_POI_Image_Converter.exe"; Description: "Launch POV POI Image Converter"; Flags: nowait postinstall skipifsilent
```

### Build the Installer

1. Open the `.iss` file in Inno Setup Compiler
2. Click "Build" → "Compile"
3. The installer will be created in `installer_output/`

### Benefits of Using Inno Setup

- Professional installation experience
- Start Menu shortcuts
- Desktop icon
- Proper uninstall support
- Version tracking
- File associations (optional)

## Troubleshooting

### Windows Defender Warnings

**Issue**: Windows Defender or antivirus software flags the executable as potentially harmful.

**Why**: PyInstaller executables are sometimes flagged as false positives because:
- They use self-extraction techniques
- Not code-signed by a trusted authority
- Relatively new/unknown file

**Solutions**:

1. **For Developers**:
   - Code sign the executable (recommended for distribution)
   - Submit to Microsoft for analysis
   - Use Inno Setup to create a proper installer

2. **For Users**:
   - This is a **false positive** - the file is safe
   - Click "More info" → "Run anyway"
   - Add exception to your antivirus
   - Verify checksum with the developer

3. **Verify the File**:
   ```bash
   # Generate checksum
   certutil -hashfile POV_POI_Image_Converter.exe SHA256
   ```

### Missing DLL Errors

**Issue**: `VCRUNTIME140.dll` or similar DLL missing.

**Solution**: Install Microsoft Visual C++ Redistributable:
- Download from: https://aka.ms/vs/17/release/vc_redist.x64.exe
- Install and restart

### Permission Issues

**Issue**: "Access denied" or "Permission denied" errors.

**Solutions**:
1. Run as Administrator (right-click → "Run as administrator")
2. Move the `.exe` to a non-protected location (e.g., Desktop or Downloads)
3. Check antivirus settings

### Application Won't Start

**Issue**: Double-clicking does nothing or shows error.

**Debugging**:

1. **Run from Command Line**:
   ```bash
   cd path\to\exe
   POV_POI_Image_Converter.exe
   ```
   This will show error messages

2. **Check Windows Event Viewer**:
   - Open Event Viewer
   - Look under Windows Logs → Application
   - Check for errors related to the application

3. **Verify File Integrity**:
   - Re-download or rebuild the executable
   - Check file size is reasonable (~15-20 MB)

### Slow First Launch

**Issue**: Application takes a long time to start the first time.

**Explanation**: This is normal behavior:
- PyInstaller extracts files to a temporary directory
- First launch: 5-10 seconds
- Subsequent launches: 1-2 seconds

**To Speed Up**:
- Use `--onedir` instead of `--onefile` (multiple files but faster)
- Accept the tradeoff (convenience vs. speed)

### Build Fails

**Issue**: `build_windows_installer.py` fails with errors.

**Common Causes and Solutions**:

1. **PyInstaller not installed**:
   ```bash
   pip install pyinstaller>=5.0
   ```

2. **Dependencies missing**:
   ```bash
   pip install -r requirements.txt
   ```

3. **Python version too old**:
   - Upgrade to Python 3.7 or higher

4. **Path issues**:
   - Make sure you're in the `examples/` directory
   - Use absolute paths if needed

5. **Permissions**:
   - Run Command Prompt as Administrator
   - Disable antivirus temporarily during build

### Image Converter Doesn't Work in Executable

**Issue**: GUI opens but image conversion fails.

**Solutions**:
1. Check that `image_converter.py` is included in build
2. Verify hidden imports are specified correctly
3. Rebuild with `--debug` flag to see detailed errors:
   ```bash
   pyinstaller --onefile --debug=all image_converter_gui.py
   ```

## Technical Details

### PyInstaller Configuration

The build uses the following PyInstaller options:

- `--onefile`: Bundle everything into a single executable
- `--windowed` / `--noconsole`: Hide console window for GUI app
- `--name`: Set executable name to `POV_POI_Image_Converter`
- `--add-data`: Include `image_converter.py` module
- `--hidden-import`: Explicitly include PIL and tkinter modules
- `--exclude-module`: Exclude unnecessary large modules

### What Gets Bundled

The executable includes:
- Python interpreter
- All Python standard library modules used
- Pillow (PIL) library
- tkinter GUI framework
- `image_converter.py` module
- `image_converter_gui.py` application code

### Size Optimization

Current executable size: ~15-20 MB

To reduce size:
1. Use `--exclude-module` for unused libraries
2. Use UPX compression (enabled by default)
3. Remove debug symbols with `--strip`
4. Consider `--onedir` for faster startup (but multiple files)

### Security Considerations

#### For Developers

1. **Code Signing** (Recommended):
   - Purchase code signing certificate
   - Sign the executable: `signtool sign /f cert.pfx /p password POV_POI_Image_Converter.exe`
   - Prevents Windows Defender warnings

2. **Checksum Distribution**:
   - Generate SHA256 checksum
   - Publish alongside the executable
   - Users can verify integrity

3. **Open Source**:
   - Keep source code available
   - Allow users to build themselves
   - Transparency builds trust

#### For Users

1. Download only from trusted sources
2. Verify checksum if provided
3. Build from source if concerned about security

### Build Environment

- **Recommended**: Build on the oldest Windows version you want to support
- **Python**: Use official python.org installer (not Windows Store version)
- **Virtual Environment**: Recommended to avoid dependency conflicts:
  ```bash
  python -m venv build_env
  build_env\Scripts\activate
  pip install -r requirements.txt
  pip install -r installer_requirements.txt
  python build_windows_installer.py
  ```

## Code Signing

For professional distribution, code signing is recommended:

### Why Code Sign?

- Eliminates Windows Defender warnings
- Establishes trust with users
- Provides authenticity verification
- Required for some enterprise deployments

### How to Code Sign

1. **Get a Certificate**:
   - Purchase from a Certificate Authority (CA)
   - Options: DigiCert, Sectigo, GlobalSign
   - Cost: ~$100-500/year

2. **Sign the Executable**:
   ```bash
   signtool sign /f certificate.pfx /p password /t http://timestamp.digicert.com POV_POI_Image_Converter.exe
   ```

3. **Verify Signature**:
   ```bash
   signtool verify /pa POV_POI_Image_Converter.exe
   ```

### Free Alternative

- **Windows Defender Application Control**: Submit to Microsoft for analysis
- **Open Source Projects**: Some CAs offer free certificates for OSS

## Additional Resources

- **PyInstaller Documentation**: https://pyinstaller.org/
- **Inno Setup Documentation**: https://jrsoftware.org/ishelp/
- **Code Signing Guide**: https://docs.microsoft.com/en-us/windows/win32/seccrypto/cryptography-tools
- **Project Repository**: [Your GitHub Repository]

## Support

If you encounter issues:

1. Check this troubleshooting guide
2. Search existing GitHub issues
3. Open a new issue with:
   - Error messages
   - Build log output
   - Python version
   - Windows version
   - Steps to reproduce

---

**Built with ❤️ by the POV POI community**
