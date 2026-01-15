@echo off
REM Build script for compiling Teensy 4.1 firmware using Arduino CLI
REM Generates HEX file for Teensy Loader

echo ======================================================
echo Building Teensy 4.1 Firmware with Arduino CLI
echo ======================================================
echo.

REM Check if arduino-cli is installed
where arduino-cli >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: arduino-cli is not installed!
    echo.
    echo To install Arduino CLI:
    echo   Download from: https://github.com/arduino/arduino-cli/releases
    echo   Or use winget: winget install ArduinoSA.CLI
    echo.
    exit /b 1
)

for /f "tokens=*" %%i in ('arduino-cli version') do set ARDUINO_VERSION=%%i
echo [32m✓[0m Arduino CLI found: %ARDUINO_VERSION%
echo.

REM Check if Teensy platform is installed
echo Checking Teensy platform...
arduino-cli core list | findstr /C:"teensy:avr" >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [33m⚠[0m  Teensy platform not found. Installing...
    arduino-cli config add board_manager.additional_urls https://www.pjrc.com/teensy/package_teensy_index.json
    arduino-cli core update-index
    arduino-cli core install teensy:avr
    echo [32m✓[0m Teensy platform installed
) else (
    echo [32m✓[0m Teensy platform found
)
echo.

REM Check if FastLED library is installed
echo Checking FastLED library...
arduino-cli lib list | findstr /C:"FastLED" >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [33m⚠[0m  FastLED library not found. Installing...
    arduino-cli lib install FastLED
    echo [32m✓[0m FastLED library installed
) else (
    echo [32m✓[0m FastLED library found
)
echo.

REM Create output directory
if not exist teensy_firmware\build mkdir teensy_firmware\build

echo Compiling firmware...
echo.

REM Compile the firmware
arduino-cli compile --fqbn teensy:avr:teensy41:usb=serial,speed=600,opt=o2std teensy_firmware\teensy_firmware.ino --output-dir teensy_firmware\build

echo.
echo ======================================================
echo Build Complete!
echo ======================================================
echo.
echo The HEX file is ready to load with Teensy Loader:
echo   Location: teensy_firmware\build\teensy_firmware.ino.hex
echo.
echo To upload using Teensy Loader:
echo   1. Open Teensy Loader application
echo   2. File ^> Open HEX File
echo   3. Select: teensy_firmware\build\teensy_firmware.ino.hex
echo   4. Press the button on your Teensy board
echo   5. Click 'Program' in Teensy Loader
echo.
echo Or upload directly via CLI:
echo   arduino-cli upload --fqbn teensy:avr:teensy41 --port COM3 teensy_firmware\teensy_firmware.ino
echo.
echo ======================================================
