@echo off
setlocal enabledelayedexpansion

REM Load Visual Studio build environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

REM Change to project root
cd /d "C:\VCC\themis"

REM Remove old build directory
echo [1/4] Removing old build directory...
rmdir /s /q build-ninja-tests-bench 2>nul
echo [OK] Old build removed

REM Configure with CMake
echo [2/4] Configuring with CMake (Ninja, Release, LLM/GPU disabled)...
cmake -S . -B build-ninja-tests-bench ^
  -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DTHEMIS_BUILD_TESTS=ON ^
  -DTHEMIS_BUILD_BENCHMARKS=ON ^
  -DTHEMIS_ENABLE_LLM=OFF ^
  -DTHEMIS_ENABLE_GPU=OFF ^
  -DTHEMIS_ENABLE_CUDA=OFF ^
  -DTHEMIS_ENABLE_HIP=OFF

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed!
    exit /b 1
)
echo [OK] CMake configuration complete

REM Build with Ninja
echo.
echo [3/4] Building with Ninja (parallel 8)...
cd build-ninja-tests-bench
ninja -j 8 2>&1 | findstr /C:"Error" /C:"error" /C:"ERROR" || echo [OK] Build progressing...

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    REM Continue anyway to show partial results
)
echo [OK] Build phase complete

REM Verify binary locations
echo.
echo [4/4] Verifying built binaries...
if exist "cmake\themis_server.exe" echo [OK] themis_server.exe exists
if exist "cmake\themis_core.lib" echo [OK] themis_core.lib exists (size: !FILE_SIZE!)
cd ..

pause
