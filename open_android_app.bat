@echo off
REM Quick Start Script for Nebula Poi Android App (Windows)

echo ==========================================
echo Nebula Poi Android App - Quick Start
echo ==========================================
echo.

REM Check common Android Studio installation paths
set "STUDIO_PATH="

if exist "C:\Program Files\Android\Android Studio\bin\studio64.exe" (
    set "STUDIO_PATH=C:\Program Files\Android\Android Studio\bin\studio64.exe"
)

if exist "%LOCALAPPDATA%\Programs\Android Studio\bin\studio64.exe" (
    set "STUDIO_PATH=%LOCALAPPDATA%\Programs\Android Studio\bin\studio64.exe"
)

if defined STUDIO_PATH (
    echo Android Studio found!
    echo.
    echo Opening project in Android Studio...
    start "" "%STUDIO_PATH%" "%~dp0POVPoiApp"
    goto :end
)

REM If Android Studio not found, provide instructions
echo Android Studio not detected on your system.
echo.
echo To build the Android app:
echo.
echo 1. Install Android Studio from:
echo    https://developer.android.com/studio
echo.
echo 2. Open Android Studio
echo.
echo 3. Select 'Open an existing project'
echo.
echo 4. Navigate to and select the 'POVPoiApp' directory
echo.
echo 5. Wait for Gradle sync to complete
echo.
echo 6. Click 'Run' to build and install on your device
echo.
echo ==========================================
echo For detailed instructions, see:
echo   POVPoiApp\README.md
echo ==========================================

:end
echo.
pause
