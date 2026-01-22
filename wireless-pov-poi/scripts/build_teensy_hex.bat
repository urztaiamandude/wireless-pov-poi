@echo off
REM Build script for compiling Teensy 4.1 firmware to HEX format
REM This HEX file can be loaded using the Teensy Loader application

echo ======================================================
echo Building Teensy 4.1 Firmware for Teensy Loader
echo ======================================================
echo.

REM Check if PlatformIO is installed
where pio >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: PlatformIO is not installed!
    echo.
    echo To install PlatformIO:
    echo   pip install platformio
    echo.
    echo Or install via the PlatformIO IDE:
    echo   https://platformio.org/install/ide
    exit /b 1
)

for /f "tokens=*" %%i in ('pio --version') do set PIO_VERSION=%%i
echo [32m✓[0m PlatformIO found: %PIO_VERSION%
echo.

REM Clean previous build
echo Cleaning previous build...
pio run -e teensy41 --target clean

echo.
echo Building firmware...
echo.

REM Build the firmware
pio run -e teensy41

echo.
echo ======================================================
echo Build Complete!
echo ======================================================
echo.
echo The HEX file is ready to load with Teensy Loader:
echo   Location: build_output\teensy41_firmware.hex
echo.
echo To upload using Teensy Loader:
echo   1. Open Teensy Loader application
echo   2. File ^> Open HEX File
echo   3. Select: build_output\teensy41_firmware.hex
echo   4. Press the button on your Teensy board
echo   5. Click 'Program' in Teensy Loader
echo.
echo ======================================================
