@echo off
REM Initialize Visual Studio environment and build with Ninja
setlocal enabledelayedexpansion

REM Set Visual Studio paths
set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional"
set "VSDEVCMD=%VS_PATH%\Common7\Tools\VsDevCmd.bat"

REM Call VSDevCmd to setup environment
call "%VSDEVCMD%" -arch=x64 >nul 2>&1

REM Verify CL.EXE is in PATH
where cl.exe >nul 2>&1
if errorlevel 1 (
    echo ERROR: cl.exe not found in PATH after VSDevCmd initialization
    exit /b 1
)

REM Navigate to project root
cd /d C:\VCC\themis

echo [*] Configuring Themis with Ninja + Tests + Benchmarks...
cmake -S . -B build-ninja-full ^
    -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DTHEMIS_BUILD_TESTS=ON ^
    -DTHEMIS_BUILD_BENCHMARKS=ON

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

echo.
echo [+] Configuration successful
echo [*] Building themis_server, tests, and benchmarks...
echo.

cmake --build build-ninja-full --config Release --parallel 8 --target themis_server

if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo [+] Build completed successfully
echo [*] Binary: build-ninja-full\cmake\Release\themis_server.exe
echo.
