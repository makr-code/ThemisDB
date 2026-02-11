@echo off
setlocal enabledelayedexpansion

REM Activate Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo

REM Change to project directory
cd /d C:\VCC\themis

REM Remove old build directory
if exist build-msvc-ninja-release (
    echo Removing old build directory...
    rmdir /s /q build-msvc-ninja-release || goto :error
)

REM Configure with CMake
echo Configuring project...
cmake -S . -B build-msvc-ninja-release -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DTHEMIS_BUILD_TESTS=OFF ^
    -DTHEMIS_BUILD_BENCHMARKS=OFF ^
    -DTHEMIS_ENABLE_LLM=ON || goto :error

REM Build
echo Building project...
cmake --build build-msvc-ninja-release --config Release --parallel 4 || goto :error

echo Build completed successfully!
exit /b 0

:error
echo Build failed with error code %ERRORLEVEL%
exit /b %ERRORLEVEL%
