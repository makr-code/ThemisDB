@echo off
REM Build themis_server using Visual Studio generator (MSVC) which handles its own environment
setlocal enabledelayedexpansion

echo [1/4] Initializing Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64
if errorlevel 1 (
    echo ERROR: VsDevCmd failed
    exit /b 1
)

cd /d C:\VCC\themis
if errorlevel 1 (
    echo ERROR: Failed to change directory to C:\VCC\themis
    exit /b 1
)

echo [2/4] Configuring with Visual Studio 17 2022 generator...
if exist build-msvc-vs (
    rmdir /s /q build-msvc-vs
)
cmake -S . -B build-msvc-vs -G "Visual Studio 17 2022" -A x64 ^
    -DVCPKG_ROOT="C:\VCC\themis\vcpkg" ^
    -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
    -DTHEMIS_BUILD_TESTS=OFF ^
    -DTHEMIS_BUILD_BENCHMARKS=OFF
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

echo [3/4] Building themis_server...
cmake --build build-msvc-vs --config Release --target themis_server --parallel 8
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo [4/4] Verifying binary...
if exist build-msvc-vs\Release\themis_server.exe (
    echo SUCCESS: themis_server.exe built
    build-msvc-vs\Release\themis_server.exe --version
    exit /b 0
) else (
    echo ERROR: themis_server.exe not found
    exit /b 1
)
