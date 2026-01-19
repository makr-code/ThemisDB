@echo off
setlocal enabledelayedexpansion

REM Initialize Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

REM Change to project directory
cd /d C:\VCC\themis

REM Clean up old build
if exist build-ninja-core (
    rmdir /s /q build-ninja-core
)

REM Configure CMake
echo [1/3] Configuring CMake...
cmake -S . -B build-ninja-core -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DBUILD_SHARED_LIBS=ON ^
    -DTHEMIS_BUILD_TESTS=OFF ^
    -DTHEMIS_ENABLE_LLM=OFF ^
    -DTHEMIS_BUILD_BENCHMARKS=OFF

if !errorlevel! neq 0 (
    echo [ERROR] CMake configuration failed
    exit /b 1
)

REM Build themis_core
echo [2/3] Building themis_core...
cmake --build build-ninja-core --target themis_core --config Release --parallel 4

if !errorlevel! neq 0 (
    echo [ERROR] Build failed
    exit /b 1
)

echo [3/3] Build successful!
cd build-ninja-core\cmake
themis_server.exe --version
