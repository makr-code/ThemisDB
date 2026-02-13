@echo off
REM Initialize Visual Studio development environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo

REM Set build directory
set BUILD_DIR=build-ninja-clean-lvm-gpu
if exist %BUILD_DIR% (
    echo Removing old build directory...
    rmdir /s /q %BUILD_DIR%
)

echo.
echo ================================
echo CMAKE CONFIGURE
echo ================================
mkdir %BUILD_DIR%
cd %BUILD_DIR%

cmake -S .. -B . ^
    -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DTHEMIS_BUILD_TESTS=ON ^
    -DTHEMIS_BUILD_BENCHMARKS=ON ^
    -DTHEMIS_ENABLE_LLM=ON ^
    -DTHEMIS_ENABLE_GPU=ON

if errorlevel 1 (
    echo CMAKE CONFIGURE FAILED
    exit /b 1
)

echo.
echo ================================
echo CMAKE BUILD - TESTS ONLY
echo ================================
cmake --build . --config Release --parallel 4 --target themis_tests

if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)

echo.
echo ================================
echo BUILD SUCCESSFUL
echo ================================
echo Executable: %BUILD_DIR%\themis_tests.exe
pause
