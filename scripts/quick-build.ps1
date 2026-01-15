#!/usr/bin/env pwsh
param(
    [int]$Jobs = 8,
    [bool]$CleanBuild = $false
)

$ErrorActionPreference = "Stop"

$RepoRoot = "C:\VCC\themis"
$BuildDir = "$RepoRoot\build-ninja"
$VcVarsAll = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"

# Load VS environment
Write-Host "=== Loading Visual Studio Environment ===" -ForegroundColor Cyan
$env:VSCMD_ARG_TGT_ARCH = "x64"
$env:VSCMD_ARG_HOST_ARCH = "x64"
cmd /c "`"$VcVarsAll`" x64 && set" | ForEach-Object {
    if ($_ -match "^(.*?)=(.*)$") {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2])
    }
}

# Clean
if ($CleanBuild) {
    Write-Host "=== Cleaning ===" -ForegroundColor Cyan
    Remove-Item $BuildDir -Recurse -Force -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null

# Configure
Write-Host "=== Configuring CMake ===" -ForegroundColor Cyan
Push-Location $RepoRoot
$env:VCPKG_ROOT = "$RepoRoot\vcpkg"

cmake -S . -B $BuildDir `
    -G Ninja `
    "-DCMAKE_BUILD_TYPE=Release" `
    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
    -DTHEMIS_BUILD_TESTS=OFF `
    -DTHEMIS_BUILD_BENCHMARKS=OFF `
    -DTHEMIS_BUILD_DOCS_DB=OFF `
    -DTHEMIS_ENABLE_LLM=OFF

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

# Build
Write-Host "=== Building with Ninja ===" -ForegroundColor Cyan
Push-Location $BuildDir
ninja -j $Jobs

$result = $LASTEXITCODE

if ($result -eq 0) {
    Write-Host "SUCCESS: Build completed successfully!" -ForegroundColor Green
} else {
    Write-Host "ERROR: Build failed" -ForegroundColor Red
}

Pop-Location
Pop-Location
exit $result
