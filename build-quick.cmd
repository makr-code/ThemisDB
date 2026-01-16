@echo off
REM Quick build with ErrorRegistry Fix

call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

cd /d C:\VCC\themis

echo [1/3] Configuring CMake...
cmake -S . -B build-ninja -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DTHEMIS_BUILD_TESTS=OFF -DTHEMIS_BUILD_BENCHMARKS=OFF

if %ERRORLEVEL% neq 0 (
  echo CMake configuration failed
  exit /b 1
)

echo [2/3] Building themis_server...
cmake --build build-ninja --target themis_server --parallel 8

if %ERRORLEVEL% neq 0 (
  echo Build failed
  exit /b 1
)

echo [3/3] Testing binary...
cd build-ninja\cmake
themis_server.exe --version
set EXITCODE=%ERRORLEVEL%

echo Build completed with exit code: %EXITCODE%
exit /b %EXITCODE%
