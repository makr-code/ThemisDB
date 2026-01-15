@echo off
setlocal enabledelayedexpansion

echo === Windows Build - Fresh Configuration ===
echo.

cd /d C:\VCC\themis

echo [1/4] Loading Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

echo [2/4] Removing old build...
rmdir /s /q build-ninja 2>nul

echo [3/4] Reconfiguring CMake...
cmake -S . -B build-ninja -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DBUILD_SHARED_LIBS=ON ^
    -DTHEMIS_BUILD_TESTS=ON ^
    -DTHEMIS_BUILD_BENCHMARKS=ON ^
    -DCMAKE_BUILD_TYPE=Release

echo [4/4] Building with Ninja (serial)...
cmake --build build-ninja --parallel 1

if errorlevel 0 (
    echo.
    echo === Build Status ===
    if exist build-ninja\cmake\themis_server.exe (
        echo SUCCESS: Main binary created
    ) else (
        echo Note: Check tests/ and benchmarks/ subdirectories for outputs
    )
)
