@echo off
REM Initialize VS2022 Developer Environment and run complete CMake+Ninja build

set "VSPATH=C:\Program Files\Microsoft Visual Studio\2022\Professional"
set "VCPATH=%VSPATH%\VC"

REM Initialize VS Developer Command Prompt environment
call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo

REM Set additional environment variables
set "VCToolsInstallDir=%VCPATH%\Tools\MSVC\14.44.35207\"
set "INCLUDE=%VCPATH%\Tools\MSVC\14.44.35207\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um;%INCLUDE%"
set "LIB=%VCPATH%\Tools\MSVC\14.44.35207\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64;%LIB%"
set "LIBPATH=%VCPATH%\Tools\MSVC\14.44.35207\lib\x64;%LIBPATH%"

cd /d C:\VCC\themis

echo ╔════════════════════════════════════════════════════════════════╗
echo ║  ThemisDB: CMake Configure + Ninja Build (with LoRA)         ║
echo ╚════════════════════════════════════════════════════════════════╝
echo.

REM Clean old build
if exist build-msvc-ninja-release (
    echo [*] Cleaning old build directory...
    rmdir /s /q build-msvc-ninja-release
)
echo.

REM Configure with CMake
echo [1/3] Configuring with CMake...
cmake -S . -B build-msvc-ninja-release -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DTHEMIS_BUILD_TESTS=ON ^
  -DTHEMIS_ENABLE_LLM=ON ^
  -DTHEMIS_ENABLE_GPU=OFF

if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    exit /b 1
)
echo [✓] CMake configuration succeeded
echo.

REM Build with Ninja (still in VsDevCmd environment)
echo [2/3] Building with Ninja (4 jobs)...
cd build-msvc-ninja-release
ninja -j 4 themis_tests

if errorlevel 1 (
    echo ERROR: Build failed!
    cd ..
    exit /b 1
)
echo [✓] Build succeeded
echo.

cd ..

REM Check for output
echo [3/3] Verifying build output...
if exist build-msvc-ninja-release\themis_tests.exe (
    echo [✓] themis_tests.exe created
    echo.
    echo ╔════════════════════════════════════════════════════════════════╗
    echo ║  BUILD SUCCESSFUL!                                           ║
    echo ╚════════════════════════════════════════════════════════════════╝
    echo.
    echo Next: Run LoRA tests
    echo   .\build-msvc-ninja-release\themis_tests.exe --gtest_filter="*LoRA*"
    exit /b 0
) else (
    echo ERROR: themis_tests.exe not found!
    exit /b 1
)
