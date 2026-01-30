# Image Converter Builds

The image converter tools live in `examples/`.

## Python GUI (cross-platform)

Install dependencies:

```bash
cd examples
pip install -r requirements.txt
```

Run the GUI:

```bash
python image_converter_gui.py
```

## Windows executable (no Python required)

Prerequisites:
- Windows 7 or later
- Python 3.7+

Build steps:

```bash
cd examples
pip install -r installer_requirements.txt
python build_windows_installer.py
```

Output:
`examples/dist/POV_POI_Image_Converter.exe`

## Manual PyInstaller build (optional)

```bash
pyinstaller --onefile --windowed --name=POV_POI_Image_Converter ^
  --add-data="image_converter.py;." ^
  --hidden-import=PIL --hidden-import=PIL.Image ^
  --hidden-import=PIL.ImageTk --hidden-import=PIL.ImageEnhance ^
  image_converter_gui.py
```

## Troubleshooting

- Missing Pillow: `pip install Pillow`
- GUI not opening on Linux: `sudo apt-get install python3-tk`
