@echo off
REM Build, flash, and run all hardware tests (Windows).
REM Usage:
REM   scripts\build_flash_test.bat                     Build + flash + test
REM   scripts\build_flash_test.bat --test-only         Skip build/flash
REM   scripts\build_flash_test.bat --suite teensy      Teensy serial tests only
REM   scripts\build_flash_test.bat --list-ports        Show serial ports
REM
REM Environment variables (override auto-detection):
REM   set TEENSY_PORT=COM3
REM   set ESP32_PORT=COM15
REM   set ESP32_URL=http://192.168.4.1

cd /d "%~dp0\.."

set EXTRA_ARGS=
set TEST_ONLY=0

:parse_args
if "%~1"=="" goto :done_args
if "%~1"=="--test-only" (
    set TEST_ONLY=1
    shift
    goto :parse_args
)
set EXTRA_ARGS=%EXTRA_ARGS% %1
shift
goto :parse_args
:done_args

set BUILD_ARGS=
if %TEST_ONLY%==0 set BUILD_ARGS=--build --flash

set PORT_ARGS=
if defined TEENSY_PORT set PORT_ARGS=%PORT_ARGS% --teensy-port %TEENSY_PORT%
if defined ESP32_PORT  set PORT_ARGS=%PORT_ARGS% --esp32-port %ESP32_PORT%
if defined ESP32_URL   set PORT_ARGS=%PORT_ARGS% --esp32-url %ESP32_URL%

python -m scripts.test_hardware.run_tests %BUILD_ARGS% %PORT_ARGS% --report test_results.json %EXTRA_ARGS%
