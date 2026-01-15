@echo off
setlocal enabledelayedexpansion

echo === Windows Ninja Build mit Tests + Benchmarks (ohne LLM) ===
echo.

REM Load Visual Studio environment
echo [1/3] Loading Visual Studio 2022 environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

if errorlevel 1 (
    echo ERROR: Failed to load Visual Studio environment
    exit /b 1
)

echo [2/3] Configuring CMake (LLM disabled, Tests/Benchmarks enabled)...
cd /d C:\VCC\themis
cmake -S . -B build-ninja -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DBUILD_SHARED_LIBS=ON ^
    -DTHEMIS_BUILD_TESTS=ON ^
    -DTHEMIS_BUILD_BENCHMARKS=ON ^
    -DTHEMIS_ENABLE_LLM=OFF ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

echo [3/3] Building with Ninja (8 parallel jobs)...
cmake --build build-ninja --parallel 8

if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo === Build completed successfully! ===
echo.
dir build-ninja\cmake\themis_server.exe 2>nul && (
    echo Main binary: build-ninja\cmake\themis_server.exe
) || echo [Binary not found]
echo.
echo Testing files available in: build-ninja\tests\
echo Benchmark files available in: build-ninja\benchmarks\
echo.
exit /b 0
