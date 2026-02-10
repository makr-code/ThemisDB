#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build ThemisDB with proper VS2022 environment setup
.DESCRIPTION
    Initializes VS2022 Developer Environment and builds with CMake + Ninja
#>

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    
    [int]$Jobs = 8,
    
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

# ════════════════════════════════════════════════════════════════════════════
# Setup
# ════════════════════════════════════════════════════════════════════════════

$WorkspaceRoot = 'C:\VCC\themis'
$BuildDir = Join-Path $WorkspaceRoot "build-ninja-final"
$VsDevCmd = 'C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat'

Write-Host "╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  ThemisDB Build System (with LoRA Migration)                 ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path $VsDevCmd)) {
    Write-Error "VsDevCmd not found at: $VsDevCmd"
}

if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "[*] Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Path $BuildDir -Recurse -Force
    New-Item -Path $BuildDir -ItemType Directory | Out-Null
}

# ════════════════════════════════════════════════════════════════════════════
# Initialize VS Developer Environment
# ════════════════════════════════════════════════════════════════════════════

Write-Host "[1/3] Initializing VS2022 Developer Environment..." -ForegroundColor Cyan

$InitScript = @"
@echo off
call "$VsDevCmd" -arch=x64 -no_logo >nul 2>&1
powershell -NoProfile -Command {
    `$env:Path
    `$env:INCLUDE
    `$env:LIB
    `$env:LIBPATH
}
"@

$EnvOutput = & cmd /c $InitScript
$EnvLines = $EnvOutput -split '\n'
$env:Path = $EnvLines[0]
$env:INCLUDE = $EnvLines[1]
$env:LIB = $EnvLines[2]
$env:LIBPATH = $EnvLines[3]

Write-Host "  ✓ VS2022 environment initialized" -ForegroundColor Green
Write-Host "  ✓ INCLUDE paths configured" -ForegroundColor Green
Write-Host "  ✓ LIB paths configured" -ForegroundColor Green

# ════════════════════════════════════════════════════════════════════════════
# Configure with CMake
# ════════════════════════════════════════════════════════════════════════════

Write-Host ""
Write-Host "[2/3] Configuring CMake..." -ForegroundColor Cyan

Push-Location $WorkspaceRoot

$CmakeArgs = @(
    '-S', '.',
    '-B', $BuildDir,
    '-G', 'Ninja',
    "-DCMAKE_BUILD_TYPE=$Configuration",
    '-DCMAKE_TOOLCHAIN_FILE=C:\VCC\themis\vcpkg\scripts\buildsystems\vcpkg.cmake',
    '-DVCPKG_TARGET_TRIPLET=x64-windows',
    '-DTHEMIS_BUILD_TESTS=ON',
    '-DTHEMIS_ENABLE_LLM=ON'
)

& cmake @CmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed with exit code $LASTEXITCODE"
}
Write-Host "  ✓ CMake configuration complete" -ForegroundColor Green

# ════════════════════════════════════════════════════════════════════════════
# Build with Ninja
# ════════════════════════════════════════════════════════════════════════════

Write-Host ""
Write-Host "[3/3] Building with Ninja..." -ForegroundColor Cyan

Push-Location $BuildDir

$NinjaOutput = & ninja -v --jobs $Jobs 2>&1
$NinjaExitCode = $LASTEXITCODE

# Filter output for important messages
$NinjaOutput | Select-String -Pattern '^\[|error|Error|ERROR|lora_adapter|warning C' | ForEach-Object {
    if ($_ -match 'error|Error|ERROR') {
        Write-Host $_ -ForegroundColor Red
    }
    elseif ($_ -match 'warning') {
        Write-Host $_ -ForegroundColor Yellow
    }
    else {
        Write-Host $_
    }
}

if ($NinjaExitCode -ne 0) {
    Write-Host ""
    Write-Error "Ninja build failed with exit code $NinjaExitCode`nLast 30 lines of output:`n$($NinjaOutput | Select-Object -Last 30 | Out-String)"
}

Write-Host ""
Write-Host "  ✓ Build complete!" -ForegroundColor Green

Pop-Location
Pop-Location

# ════════════════════════════════════════════════════════════════════════════
# Summary
# ════════════════════════════════════════════════════════════════════════════

Write-Host ""
Write-Host "╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║  BUILD SUCCESSFUL                                             ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""
Write-Host "Build Directory: $BuildDir" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  • Run tests: $BuildDir\themis_tests.exe --gtest_filter='*LoRA*'"
Write-Host "  • Run all tests: $BuildDir\themis_tests.exe"
Write-Host ""
