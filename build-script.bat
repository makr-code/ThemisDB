@echo off
setlocal enabledelayedexpansion
cd /d C:\VCC\themis

echo [1/4] Initializing Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo

if %ERRORLEVEL% NEQ 0 (
    echo VSDevCmd initialization failed!
    exit /b 1
)

echo Environment check - checking CL.exe:
where cl.exe

echo [2/4] Cleaning old build directory...
if exist build-test-all (
    rmdir /s /q build-test-all
)
mkdir build-test-all
cd build-test-all

echo [3/4] Configuring with CMake...
cmake .. ^
    -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DTHEMIS_BUILD_TESTS=ON ^
    -DTHEMIS_BUILD_BENCHMARKS=OFF ^
    -DTHEMIS_ENABLE_LLM=ON ^
    -DTHEMIS_ENABLE_GPU=ON ^
    -DTHEMIS_ENABLE_TRACING=ON ^
    -DTHEMIS_BUILD_ENTERPRISE_PLUGINS=ON ^
    -DTHEMIS_BUILD_RPC_FRAMEWORK=ON ^
    -DTHEMIS_ENABLE_AVX2=ON

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    exit /b 1
)

echo [4/4] Building with Ninja...
ninja -j 8

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo ====================================
echo Build completed successfully!
echo ====================================
endlocal
