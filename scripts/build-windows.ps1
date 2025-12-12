#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build ThemisDB for Windows using MSVC with automatic cache update
.DESCRIPTION
    Updates vcpkg cache, then compiles with MSVC in Release mode
#>

param(
    [switch]$NoCache,
    [switch]$SkipTests,
    [switch]$Debug,
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Split-Path -Parent $scriptDir

Write-Host "=== ThemisDB Windows Build ===" -ForegroundColor Cyan

# Step 1: Update cache unless skipped
if (-not $NoCache) {
    Write-Host "`n[1/3] Updating vcpkg cache..." -ForegroundColor Cyan
    & "$scriptDir\update-vcpkg-cache.ps1" -Triplets @("x64-windows")
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Cache update failed" -ForegroundColor Red
        exit 1
    }
}

# Step 2: Configure with CMake
Write-Host "`n[2/3] Configuring CMake..." -ForegroundColor Cyan
$buildDir = "$rootDir\build-msvc"

if (Test-Path $buildDir) {
    Remove-Item $buildDir -Recurse -Force
}

cmake -S $rootDir -B $buildDir `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -DCMAKE_BUILD_TYPE=$Config `
    -DTHEMIS_BUILD_TESTS=$(if ($SkipTests) { "OFF" } else { "ON" }) `
    -DTHEMIS_BUILD_BENCHMARKS=OFF `
    -DTHEMIS_ENABLE_TRACING=$(if ($Debug) { "ON" } else { "OFF" })

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed" -ForegroundColor Red
    exit 1
}

# Step 3: Build
Write-Host "`n[3/3] Building with MSVC..." -ForegroundColor Cyan
cmake --build $buildDir --config $Config --parallel 4

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed" -ForegroundColor Red
    exit 1
}

Write-Host "`n=== Build Complete ===" -ForegroundColor Green
$binary = "$buildDir\$Config\themis_server.exe"
if (Test-Path $binary) {
    $size = (Get-Item $binary).Length / 1MB
    Write-Host "Binary: $binary ($([math]::Round($size, 2)) MB)" -ForegroundColor Green
}
