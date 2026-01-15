@echo off
setlocal enabledelayedexpansion

echo === Clean Windows Ninja Build mit Tests + Benchmarks ===
echo.

REM Load Visual Studio environment
echo [1/4] Loading Visual Studio 2022 environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

if errorlevel 1 (
    echo ERROR: Failed to load Visual Studio environment
    exit /b 1
)

echo [2/4] Removing old build-ninja directory...
cd /d C:\VCC\themis
rmdir /s /q build-ninja 2>nul || echo [Verzeichnis existiert noch nicht]

echo [3/4] Configuring CMake with Ninja...
cmake -S . -B build-ninja -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
    -DBUILD_SHARED_LIBS=ON ^
    -DTHEMIS_BUILD_TESTS=ON ^
    -DTHEMIS_BUILD_BENCHMARKS=ON ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

echo [4/4] Building with Ninja (8 parallel jobs - this may take 10-15 minutes)...
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
exit /b 0
