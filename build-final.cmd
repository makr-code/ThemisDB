@echo off
REM Final Build Script - Fix: Use Visual Studio Generator (Multi-Config) instead of Ninja
REM This ensures CMAKE_BUILD_TYPE=Release actually results in Release libraries

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   ThemisDB Windows Release Build
echo   (Multi-Config with Visual Studio)
echo ========================================
echo.

call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

cd /d C:\VCC\themis

echo [1/4] Cleaning old build...
if exist build-vs rmdir /s /q build-vs
if exist vcpkg_installed\x64-windows rmdir /s /q vcpkg_installed\x64-windows

echo [2/4] CMake: Visual Studio Generator (Multi-Config, ensures Release libs)...
cmake -S . -B build-vs ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DVCPKG_BUILD_TYPE=release ^
  -DTHEMIS_BUILD_TESTS=OFF ^
  -DTHEMIS_BUILD_BENCHMARKS=OFF

if %ERRORLEVEL% neq 0 (
  echo ERROR: CMake configuration failed
  exit /b 1
)

echo [3/4] Building themis_server (Release config)...
cmake --build build-vs --config Release --target themis_server --parallel 8

if %ERRORLEVEL% neq 0 (
  echo ERROR: Build failed
  exit /b 1
)

echo [4/4] Testing binary...
echo.

set BINARY=.\build-vs\Release\themis_server.exe

if not exist "%BINARY%" (
  echo ERROR: Binary not found at %BINARY%
  exit /b 1
)

echo Binary location: %BINARY%
echo Testing --version...
"%BINARY%" --version
set EXIT_CODE=%ERRORLEVEL%

if %EXIT_CODE% equ 0 (
  echo.
  echo ========================================
  echo SUCCESS! Binary works!
  echo ========================================
  echo Binary: %BINARY%
  echo.
) else (
  echo ERROR: --version returned code %EXIT_CODE%
  exit /b 1
)
