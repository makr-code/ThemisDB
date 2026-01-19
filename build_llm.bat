@echo off
setlocal
cd /d C:\VCC\themis

echo [1] Initialize VS environment (x64)...
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo
if errorlevel 1 (
    echo ERROR: VsDevCmd failed
    exit /b 1
)

echo [2] Clean build-ninja-llm...
rmdir /s /q build-ninja-llm 2>nul

echo [3] Configure (Ninja, Release, tests+bench+LLM)...
cmake -S . -B build-ninja-llm ^
  -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DTHEMIS_BUILD_TESTS=ON ^
  -DTHEMIS_BUILD_BENCHMARKS=ON ^
  -DTHEMIS_ENABLE_LLM=ON
if errorlevel 1 (
    echo ERROR: CMake configure failed
    exit /b 1
)

echo [4] Build...
cmake --build build-ninja-llm --parallel 8 --config Release
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

echo [5] SUCCESS: build-ninja-llm ready
exit /b 0
