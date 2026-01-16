@echo off
REM Clean Release Build für ThemisDB - konsistent Release überall
REM

setlocal enabledelayedexpansion

echo [1/5] Initializing Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64
if %ERRORLEVEL% neq 0 (
    echo ERROR: VsDevCmd failed
    exit /b 1
)

cd /d C:\VCC\themis

echo [2/5] Cleaning old build directories...
if exist build-ninja rmdir /s /q build-ninja
if exist vcpkg_installed\x64-windows rmdir /s /q vcpkg_installed\x64-windows

echo [3/5] CMake configuration (Release + Release vcpkg)...
cmake -S . -B build-ninja ^
  -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DVCPKG_BUILD_TYPE=release ^
  -DTHEMIS_BUILD_TESTS=OFF ^
  -DTHEMIS_BUILD_BENCHMARKS=OFF

if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

echo [4/5] Building themis_server (8 parallel jobs)...
cmake --build build-ninja --target themis_server --parallel 8

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)

echo [5/5] Testing binary...
echo.
echo Testing: --version
.\build-ninja\cmake\themis_server.exe --version
if %ERRORLEVEL% equ 0 (
    echo SUCCESS: --version works
) else (
    echo ERROR: --version returned code %ERRORLEVEL%
    exit /b 1
)

echo.
echo Testing: --build-info
.\build-ninja\cmake\themis_server.exe --build-info | head -5
if %ERRORLEVEL% equ 0 (
    echo SUCCESS: --build-info works
) else (
    echo WARNING: --build-info returned code %ERRORLEVEL%
)

echo.
echo ========================================
echo BUILD COMPLETE - All Release
echo Binary: .\build-ninja\cmake\themis_server.exe
echo ========================================
