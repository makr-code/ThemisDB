@echo off
REM Build script that activates MSVC environment and runs Ninja in the same process

setlocal enabledelayedexpansion

REM Activate VS 2022 Developer Environment
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo

REM Verify environment
echo.
echo === MSVC Environment Check ===
where cl.exe
echo Include paths: !INCLUDE!
echo.

REM Change to build directory and run Ninja
cd /d C:\VCC\themis\build-ninja-llm-gpu
echo === Building themis_tests ===
ninja themis_tests

REM Check result
if %ERRORLEVEL% EQU 0 (
    echo.
    echo === Build SUCCESS ===
    dir cmake\tests\themis_tests.exe
) else (
    echo.
    echo === Build FAILED ===
    exit /b 1
)

endlocal
