@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo
cd /d C:\VCC\themis

REM Rebuild the build directory
if exist build-ninja-llm-gpu rmdir /s /q build-ninja-llm-gpu
mkdir build-ninja-llm-gpu

REM Configure
cmake -S . -B build-ninja-llm-gpu -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DTHEMIS_BUILD_TESTS=ON ^
  -DTHEMIS_ENABLE_LLM=ON

if %ERRORLEVEL% neq 0 (
  echo CMake configuration failed
  exit /b 1
)

REM Build
cmake --build build-ninja-llm-gpu --config Release --parallel 4 --target themis_tests

pause
