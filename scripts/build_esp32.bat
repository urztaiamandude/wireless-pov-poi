@echo off
REM Build ESP32-S3 firmware
set ROOT=%~dp0..
cd /d "%ROOT%\esp32_firmware"
pio run -e esp32
exit /b %ERRORLEVEL%
