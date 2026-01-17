@echo off
setlocal enabledelayedexpansion

REM Load Visual Studio build environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

REM Add Windows SDK library path
set "LIB=C:\Program Files (x86)\Windows Kits\10\lib\10.0.26100.0\um\x64;!LIB!"

REM Change to project root
cd /d "C:\VCC\themis"

REM Remove old build directory
echo [1/5] Removing old build directory...
rmdir /s /q build-ninja-tests-bench 2>nul
echo [OK] Old build removed

REM Configure with CMake
echo.
echo [2/5] Configuring with CMake (Ninja, Release, Tests+Benchmarks, Auto-Enable Phase 1 Perf Features)...
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
  -DTHEMIS_ENABLE_HIP=OFF ^
  -DTHEMIS_PERF_AUTO_ENABLE=ON

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed!
    exit /b 1
)
echo [OK] CMake configuration complete

REM Build with Ninja
echo.
echo [3/5] Building with Ninja (parallel 8)...
cd build-ninja-tests-bench
ninja -j 8 2>&1

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    cd ..
    exit /b 1
)
echo [OK] Build complete

REM Verify binaries
echo.
echo [4/5] Verifying build artifacts...
cd ..
if exist "build-ninja-tests-bench\cmake\themis_server.exe" (
    echo [OK] themis_server.exe built successfully
) else (
    echo [ERROR] themis_server.exe not found!
    exit /b 1
)

if exist "build-ninja-tests-bench\cmake\themis_core.lib" (
    echo [OK] themis_core.lib built successfully
) else (
    echo [ERROR] themis_core.lib not found!
    exit /b 1
)

REM Verify benchmarks
set benchmark_count=0
for %%F in (build-ninja-tests-bench\benchmarks\benchmark_*.exe) do set /a benchmark_count+=1
echo [OK] !benchmark_count! benchmark executables built

REM Verify tests
set test_count=0
for %%F in (build-ninja-tests-bench\tests\test_*.exe) do set /a test_count+=1
echo [OK] !test_count! test executables built

REM Check build configuration
echo.
echo [5/5] Verifying performance features...
cd build-ninja-tests-bench\cmake
.\themis_server.exe --build-info 2>&1 | findstr /C:"Mimalloc" /C:"RCU Index" /C:"LIRS Cache"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] Build complete with all performance features!
) else (
    echo [WARNING] Could not verify performance feature output
)

cd ..\..
pause
