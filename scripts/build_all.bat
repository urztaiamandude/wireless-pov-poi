@echo off
REM Build both Teensy and ESP32-S3 firmware for Nebula Poi
REM Run from project root: scripts\build_all.bat

set ROOT=%~dp0..
cd /d "%ROOT%"

echo ========================================
echo Building Teensy 4.1 firmware...
echo ========================================
pio run -e teensy41
if %ERRORLEVEL% NEQ 0 (
    echo Teensy build FAILED
    exit /b 1
)
echo Teensy build OK
echo.

echo ========================================
echo Building ESP32-S3 firmware...
echo ========================================
cd esp32_firmware
pio run -e esp32
if %ERRORLEVEL% NEQ 0 (
    echo ESP32 build FAILED
    cd ..
    exit /b 1
)
cd ..
echo ESP32 build OK
echo.

echo ========================================
echo Both builds completed successfully!
echo ========================================
