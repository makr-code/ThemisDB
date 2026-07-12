# Setup MSVC environment and configure CMake
$ErrorActionPreference = 'Stop'

# Path to vcvarsall.bat
$vcvarsPath = 'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat'

# Check if vcvarsall exists
if (-not (Test-Path $vcvarsPath)) {
    Write-Error "vcvarsall.bat not found at $vcvarsPath"
    exit 1
}

Write-Host "Setting up MSVC environment..." -ForegroundColor Green

# Use cmd.exe to run vcvarsall and then cmake in the same environment
$cmd = @"
@echo off
setlocal enabledelayedexpansion

REM Call vcvarsall
call "$vcvarsPath" x64

REM Print environment info for debugging
echo.
echo LIB variable:
echo !LIB!
echo.

REM Change to project directory
cd /d c:\Projects\ThemisDB

REM Remove old build
echo Removing old build directory...
if exist build-msvc-windows-release rmdir /s /q build-msvc-windows-release

REM Run cmake
echo.
echo Running CMake configuration...
cmake --preset windows-release

echo.
echo Configuration complete.
endlocal
"@

# Write batch file
$batchFile = 'c:\Projects\ThemisDB\cmake_setup.bat'
$cmd | Out-File -FilePath $batchFile -Encoding ASCII

Write-Host "Running batch file: $batchFile" -ForegroundColor Green
& cmd /c $batchFile

Write-Host "Done!" -ForegroundColor Green
