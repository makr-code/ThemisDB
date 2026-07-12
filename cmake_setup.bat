@echo off
setlocal enabledelayedexpansion

REM Call vcvarsall
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Set Windows SDK paths manually in case vcvarsall didn't set them properly
if "!LIB!" == "" (
    echo Setting Windows SDK paths manually...
    set "LIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64;C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\lib\x64"
)

REM Print environment info for debugging
echo.
echo LIB variable:
echo !LIB!
echo.

REM Change to project directory
cd /d c:\Projects\ThemisDB

REM Remove old build
echo Removing old build directory...
if exist build-msvc-windows-release rmdir /s /q build-msvc-windows-release

REM Run cmake
echo.
echo Running CMake configuration...
cmake --preset windows-release

echo.
echo Configuration complete.
endlocal
