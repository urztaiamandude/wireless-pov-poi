@echo off
REM Build and upload ESP32-S3 firmware to COM8
echo Building ESP32-S3 firmware...
cd /d "%~dp0"
pio run -e esp32s3
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b %ERRORLEVEL%
)
echo.
echo Uploading to COM8...
pio run -e esp32s3 --target upload --upload-port COM8
if %ERRORLEVEL% NEQ 0 (
    echo Upload failed!
    pause
    exit /b %ERRORLEVEL%
)
echo.
echo Upload complete!
pause
