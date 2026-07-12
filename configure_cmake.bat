@echo off
REM Configure ThemisDB CMake with MSVC environment
setlocal enabledelayedexpansion

echo Setting up MSVC environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

echo.
echo Configuring CMake...
cd /d c:\Projects\ThemisDB
cmake --preset windows-release

echo.
echo Configuration complete. Press any key to exit.
pause
