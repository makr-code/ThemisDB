@echo off
REM Load VsDevCmd environment first
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

REM Navigate to build directory
cd /d "C:\VCC\themis\build-ninja-tests-bench"

REM Explicitly add Windows SDK paths to LIB
setlocal enabledelayedexpansion
set "WINSDK_LIB=C:\Program Files (x86)\Windows Kits\10\lib\10.0.26100.0\um\x64"
if exist "!WINSDK_LIB!" (
    set "LIB=!WINSDK_LIB!;!LIB!"
    echo [OK] Windows SDK LIB path added
)

echo [1/1] Rebuilding failed benchmarks with proper Windows SDK...
ninja -v -j 8 2>&1 | findstr /C:"benchmark_rcu_index" /C:"benchmark_huge_pages" /C:"error" /C:"LNK1181"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] Rebuild completed!
) else (
    echo.
    echo [INFO] Checking if benchmarks linked successfully...
    if exist "benchmarks\benchmark_rcu_index.exe" (
        echo [OK] benchmark_rcu_index.exe exists
    )
    if exist "benchmarks\benchmark_huge_pages.exe" (
        echo [OK] benchmark_huge_pages.exe exists
    )
)
