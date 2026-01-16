@echo off
setlocal enabledelayedexpansion

REM Load VsDevCmd environment with proper Windows SDK paths
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

echo.
echo [DEBUG] LIB environment variable:
echo !LIB!
echo.

REM Check if shlwapi.lib exists
if exist "%WindowsSDKLibPath%shlwapi.lib" (
    echo [OK] shlwapi.lib found at %WindowsSDKLibPath%shlwapi.lib
) else (
    echo [WARNING] shlwapi.lib not found, searching...
    for /r "C:\Program Files (x86)" %%A in (shlwapi.lib) do (
        echo [FOUND] %%A
        set "SHLWAPI_PATH=%%~dpA"
    )
    if defined SHLWAPI_PATH (
        echo [INFO] Found shlwapi.lib in: !SHLWAPI_PATH!
        set "LIB=!SHLWAPI_PATH!;!LIB!"
    )
)

REM Navigate to build directory
cd /d "C:\VCC\themis\build-ninja-tests-bench"

echo.
echo [1/2] Cleaning previous build artifacts...
REM ninja -t clean
REM Alternatively just remove link files
del /s *.exe.link* 2>nul

echo.
echo [2/2] Rebuilding with Ninja (parallel 8)...
ninja --verbose 2>&1

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] Build completed! Exit code: 0
    echo Binaries location: C:\VCC\themis\build-ninja-tests-bench\
) else (
    echo.
    echo [FAILED] Build failed with exit code: %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

pause
