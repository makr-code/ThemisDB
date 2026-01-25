@echo off
setlocal enabledelayedexpansion

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo

REM Clean and create fresh build directory  
if exist "build-final-test" rmdir /s /q "build-final-test"
mkdir "build-final-test"
cd /d "build-final-test"

REM Configure CMake
echo.
echo [1/3] Configuring CMake...
echo.
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DTHEMIS_BUILD_TESTS=ON ^
  -DTHEMIS_BUILD_BENCHMARKS=ON ^
  -DTHEMIS_ENABLE_LLM=ON ^
  -DTHEMIS_ENABLE_GPU=ON ^
  -DCMAKE_TOOLCHAIN_FILE="..\vcpkg\scripts\buildsystems\vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  ..

if !errorlevel! neq 0 (
  echo CMake configuration failed!
  exit /b 1
)

REM Build
echo.
echo [2/3] Building...
echo.
cmake --build . --parallel 8 --config Release

if !errorlevel! neq 0 (
  echo Build failed!
  exit /b 1
)

echo.
echo [3/3] Build complete!
echo.
cd ..
