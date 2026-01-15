@echo off
REM Clean build of themis_server (without tests and benchmarks)

setlocal enabledelayedexpansion

echo [1/4] Initializing Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

cd /d C:\VCC\themis

echo [2/4] Cleaning old build directory...
if exist build-ninja-clean (
    rmdir /s /q build-ninja-clean
)

echo [3/4] Configuring with CMake...
cmake -S . -B build-ninja-clean ^
    -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DBUILD_SHARED_LIBS=ON ^
    -DTHEMIS_BUILD_TESTS=OFF ^
    -DTHEMIS_BUILD_BENCHMARKS=OFF ^
    -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

echo [4/4] Building themis_server with Ninja...
cmake --build build-ninja-clean --target themis_server --parallel 8

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo === BUILD SUCCESS ===
echo Binary: build-ninja-clean\cmake\themis_server.exe
echo.

endlocal
