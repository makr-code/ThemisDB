@echo off
setlocal enabledelayedexpansion

REM Setup MSVC environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

cd /d c:\Projects\ThemisDB

REM Remove old build directories
if exist build-msvc-windows-release rmdir /s /q build-msvc-windows-release 2>nul
if exist build-nmake-release rmdir /s /q build-nmake-release 2>nul

echo.
echo Configuring CMake with NMake Makefiles...
cmake -G "NMake Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE=c:/Projects/ThemisDB/vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -B build-nmake-release ^
    -S . ^
    2>&1

if !errorlevel! equ 0 (
    echo.
    echo SUCCESS: CMake configuration completed!
    echo Build directory: build-nmake-release
) else (
    echo.
    echo ERROR: CMake configuration failed with code !errorlevel!
)

pause
