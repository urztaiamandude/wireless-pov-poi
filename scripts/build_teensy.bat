@echo off
REM Build Teensy 4.1 firmware
set ROOT=%~dp0..
cd /d "%ROOT%"
pio run -e teensy41
exit /b %ERRORLEVEL%
