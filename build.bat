@echo off
setlocal enabledelayedexpansion
REM Initialize VS Developer Environment via Environment Variables
set "VS_INSTALL=C:\Program Files\Microsoft Visual Studio\2022\Professional"
set "VC_TOOLS=!VS_INSTALL!\VC\Tools\MSVC\14.44.35207"

REM Set up path and include directories for Windows SDK and MSVC
set "INCLUDE=!VC_TOOLS!\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26621.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26621.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26621.0\um"
set "LIB=!VC_TOOLS!\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26621.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26621.0\um\x64"
set "PATH=!VC_TOOLS!\bin\Hostx64\x64;!VS_INSTALL!\Common7\IDE;!VS_INSTALL!\Common7\Tools;%PATH%"

REM Alternative: Use VsDevCmd
call "!VS_INSTALL!\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo

REM Configure CMake
echo [1/2] Configuring with CMake...
cd /d "C:\VCC\themis"
cmake -S . -B build-clean ^
  -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE="C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DTHEMIS_BUILD_TESTS=OFF ^
  -DTHEMIS_ENABLE_LLM=OFF ^
  -DTHEMIS_BUILD_BENCHMARKS=OFF

if errorlevel 1 (
  echo CMAKE CONFIGURE FAILED
  exit /b 1
)

REM Build
echo [2/2] Building...
cmake --build build-clean --parallel 8 --config Release

if errorlevel 1 (
  echo BUILD FAILED
  exit /b 1
)

echo BUILD SUCCESS
exit /b 0
