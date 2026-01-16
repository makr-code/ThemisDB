@echo off
REM Build Themis Server + Tests with Ninja

setlocal enabledelayedexpansion

REM Initialize VS environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

REM Navigate and build
cd /d C:\VCC\themis\build-ninja-test

echo [*] Building themis_server with Ninja...
cmake --build . --config Release --target themis_server --parallel 8

if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo [+] themis_server built successfully
echo [*] Binary: cmake\themis_server.exe
echo.
