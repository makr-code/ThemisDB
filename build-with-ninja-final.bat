@echo off
REM Build themis_server using Ninja within a properly initialized MSVC environment
setlocal enabledelayedexpansion

echo [1/5] Initializing Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64
if errorlevel 1 (
    echo ERROR: VsDevCmd failed
    exit /b 1
)

cd /d C:\VCC\themis
if errorlevel 1 (
    echo ERROR: Failed to change directory
    exit /b 1
)

echo [2/5] Cleaning old build directory...
if exist build-ninja-final (
    rmdir /s /q build-ninja-final
)

echo [3/5] Configuring with Ninja...
cmake -S . -B build-ninja-final -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DTHEMIS_BUILD_TESTS=OFF ^
    -DTHEMIS_BUILD_BENCHMARKS=OFF ^
    -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

echo [4/5] Building themis_server...
cmake --build build-ninja-final --target themis_server --parallel 8 --verbose
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo [5/5] Verifying binary and running tests...
if exist build-ninja-final\themis_server.exe (
    echo SUCCESS: themis_server.exe built at: build-ninja-final\themis_server.exe
    build-ninja-final\themis_server.exe --version
    echo exit code: !ERRORLEVEL!
    exit /b 0
) else (
    echo ERROR: themis_server.exe not found  
    exit /b 1
)
