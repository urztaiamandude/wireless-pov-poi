@echo off
REM POV SD Card Converter - Launcher for Windows
REM Double-click to run the GUI (requires Python + Pillow)

cd /d "%~dp0"
python pov_sd_converter.py
if errorlevel 1 (
    echo.
    echo If you see "python is not recognized", install Python from python.org
    echo Then run: pip install Pillow
    pause
)
