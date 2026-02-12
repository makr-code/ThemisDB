@echo off
cd /d C:\VCC\themis
echo Creating build directory...
if exist build-msvc-ninja-release rmdir /s /q build-msvc-ninja-release

echo Setting up MSVC environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo

echo Configuring CMake with Ninja...
cmake -S . -B build-msvc-ninja-release ^
  -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DTHEMIS_BUILD_TESTS=ON ^
  -DTHEMIS_BUILD_BENCHMARKS=ON ^
  -DTHEMIS_ENABLE_LLM=ON ^
  -DTHEMIS_ENABLE_GPU=ON

if %ERRORLEVEL% EQU 0 (
    echo CMake configuration successful!
    echo.
    echo Building project...
    cmake --build build-msvc-ninja-release --config Release --parallel 8
) else (
    echo CMake configuration failed!
    exit /b 1
)
