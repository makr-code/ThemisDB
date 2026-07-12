@echo off
setlocal enabledelayedexpansion

REM Setup MSVC environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

cd /d c:\Projects\ThemisDB

REM Delete old build
if exist build-msvc-windows-release rmdir /s /q build-msvc-windows-release 2>nul

echo.
echo Configuring CMake while skipping compiler tests...
cmake -DCMAKE_CXX_COMPILER_WORKS=TRUE -DCMAKE_C_COMPILER_WORKS=TRUE --preset windows-release 2>&1

if !errorlevel! equ 0 (
    echo.
    echo SUCCESS: CMake configured successfully!
    type build-msvc-windows-release\CMakeCache.txt | find "CMAKE_BUILD_TYPE" | head -3
) else (
    echo.
    echo ERROR: Configuration failed with code !errorlevel!
)

pause
