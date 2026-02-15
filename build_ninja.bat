@echo off
setlocal enabledelayedexpansion

REM Visual Studio Development Command Prompt setup
set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"

echo Initializing Visual Studio build environment...
call "%VSDEVCMD%" -arch=x64 -no_logo >nul 2>&1

echo.
echo ============================================
echo Building themis_tests (ninja)
echo ============================================
echo.

cd /d "C:\VCC\themis\build-msvc-ninja-release"
ninja -j 2 themis_tests

if errorlevel 1 (
    echo.
    echo BUILD FAILED with exit code !ERRORLEVEL!
    exit /b 1
) else (
    echo.
    echo BUILD SUCCESSFUL!
    exit /b 0
)
