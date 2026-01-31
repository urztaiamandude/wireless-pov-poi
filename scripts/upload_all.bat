@echo off
REM Build and upload both Teensy (COM10) and ESP32 (COM15)
REM Edit the COM ports below if yours differ

set TEENSY_PORT=COM10
set ESP32_PORT=COM15

set ROOT=%~dp0..
cd /d "%ROOT%"

echo ========================================
echo Building and uploading Teensy...
echo ========================================
pio run -e teensy41 --target upload
if %ERRORLEVEL% NEQ 0 (
    echo Teensy upload FAILED
    exit /b 1
)
echo Teensy upload OK
echo.

echo ========================================
echo Building and uploading ESP32...
echo ========================================
cd esp32_firmware
pio run -e esp32 --target upload --upload-port %ESP32_PORT%
if %ERRORLEVEL% NEQ 0 (
    echo ESP32 upload FAILED
    cd ..
    exit /b 1
)
cd ..
echo ESP32 upload OK
echo.

echo ========================================
echo Both uploads completed!
echo ========================================
