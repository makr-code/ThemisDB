@echo off
setlocal enabledelayedexpansion

REM Initialize MSVC environment
call "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

REM Navigate to project
cd /d C:\VCC\themis

REM Configure CMake
echo.
echo ============================================
echo Configuring CMake with Ninja generator...
echo ============================================
echo.

cmake -S . -B build-ninja -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DTHEMIS_BUILD_TESTS=ON ^
  -DTHEMIS_ENABLE_LLM=ON ^
  -DTHEMIS_BUILD_BENCHMARKS=ON

set CMAKE_EXIT=%ERRORLEVEL%

if !CMAKE_EXIT! NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed with error !CMAKE_EXIT!
    echo.
    exit /b !CMAKE_EXIT!
)

echo.
echo ============================================
echo CMake configuration completed successfully
echo ============================================
echo.
echo Now building themis_server...
echo.

ninja -C build-ninja themis_server -j 4

set BUILD_EXIT=%ERRORLEVEL%

if !BUILD_EXIT! NEQ 0 (
    echo.
    echo ERROR: Build failed with error !BUILD_EXIT!
    echo.
    exit /b !BUILD_EXIT!
)

echo.
echo ============================================
echo Build completed successfully
echo ============================================
echo.

endlocal
exit /b 0
