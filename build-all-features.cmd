@echo off
setlocal enabledelayedexpansion

echo ======================================================
echo ThemisDB: Configure + Build (Tests + Benchmarks)
echo ======================================================
echo.

REM Load Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to load Visual Studio environment
    exit /b 1
)

REM Configure
echo [1/2] Configuring with CMake...
cd /d C:\VCC\themis
cmake -S . -B build-ninja-tests-bench -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DTHEMIS_BUILD_TESTS=ON ^
    -DTHEMIS_BUILD_BENCHMARKS=ON ^
    -DTHEMIS_ENABLE_LLM=OFF ^
    -DTHEMIS_ENABLE_GPU=OFF

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

echo.
echo [2/2] Building with Ninja...
cmake --build build-ninja-tests-bench --config Release --parallel 8

if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo ======================================================
echo Build completed successfully!
echo ======================================================
echo.
echo Server: build-ninja-tests-bench\cmake\themis_server.exe
echo Tests:  build-ninja-tests-bench\tests\test_*.exe
echo Benchmarks: build-ninja-tests-bench\benchmarks\benchmark_*.exe
echo.
