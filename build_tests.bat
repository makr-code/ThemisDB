@echo off
setlocal enabledelayedexpansion
cd /d C:\VCC\themis

echo [1] Initialize VS environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo
if errorlevel 1 (
    echo ERROR: VsDevCmd failed
    exit /b 1
)

echo [2] Remove old build...
rmdir /s /q build-ninja-final 2>nul

echo [3] Configure with CMake...
cmake -S . -B build-ninja-final ^
  -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DTHEMIS_BUILD_TESTS=ON ^
  -DTHEMIS_ENABLE_LLM=OFF ^
  -DTHEMIS_BUILD_BENCHMARKS=OFF

if errorlevel 1 (
    echo ERROR: CMake configure failed
    exit /b 1
)

echo [4] Build tests...
cmake --build build-ninja-final --parallel 8 --config Release
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo [5] SUCCESS: All tests built
exit /b 0
