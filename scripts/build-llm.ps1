#!/usr/bin/env pwsh
<#
.SYNOPSIS
Build ThemisDB with LLM support using Clang/LLVM

.DESCRIPTION
This script builds ThemisDB with integrated llama.cpp LLM support.
Uses Clang compiler which has better compatibility with llama.cpp than MSVC.

.PARAMETER Jobs
Number of parallel build jobs (default: 8)

.PARAMETER CleanBuild
Remove existing build directory before building

.EXAMPLE
.\build-llm.ps1 -Jobs 12 -CleanBuild $true
#>
param(
    [int]$Jobs = 8,
    [bool]$CleanBuild = $false
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = "$RepoRoot\build-clang-llm"
$VcVarsAll = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"

# Verify Clang is installed
Write-Host "=== Verifying Clang Installation ===" -ForegroundColor Cyan
$clang = Get-Command clang -ErrorAction SilentlyContinue
if (-not $clang) {
    Write-Host "ERROR: Clang not found. Install with: choco install llvm -y" -ForegroundColor Red
    exit 1
}

# Load VS environment for linking
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

Write-Host "=== Initializing pinned llama.cpp submodule ===" -ForegroundColor Cyan
git -C $RepoRoot submodule update --init --depth 1 -- llama.cpp
if ($LASTEXITCODE -ne 0) {
    throw "Failed to initialize the pinned llama.cpp submodule."
}
cmake "-DPROJECT_SOURCE_DIR=$RepoRoot" -P "$RepoRoot\cmake\LlamaCppSubmoduleCheck.cmake"
if ($LASTEXITCODE -ne 0) {
    throw "llama.cpp submodule validation failed."
}

# Configure
Write-Host "=== Configuring CMake with Clang and LLM ===" -ForegroundColor Cyan
Push-Location $RepoRoot
$env:VCPKG_ROOT = "$RepoRoot\vcpkg"
$env:CC = "clang"
$env:CXX = "clang++"

cmake -S . -B $BuildDir `
    -G Ninja `
    "-DCMAKE_C_COMPILER=clang" `
    "-DCMAKE_CXX_COMPILER=clang++" `
    "-DCMAKE_BUILD_TYPE=Release" `
    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
    -DTHEMIS_BUILD_TESTS=OFF `
    -DTHEMIS_BUILD_BENCHMARKS=OFF `
    -DTHEMIS_BUILD_DOCS_DB=OFF `
    -DTHEMIS_ENABLE_LLM=ON

if ($LASTEXITCODE -ne 0) {
    Write-Host "✗ CMake configuration failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

# Build
Write-Host "=== Building with Ninja (Clang backend) ===" -ForegroundColor Cyan
Push-Location $BuildDir
ninja -j $Jobs

$result = $LASTEXITCODE

if ($result -eq 0) {
    Write-Host "✓ Build completed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Binary location: $BuildDir\themis_server.exe" -ForegroundColor Green
    Write-Host ""
    Write-Host "LLM Features Enabled:" -ForegroundColor Cyan
    Write-Host "  - llama.cpp integration (Ollama-style model loading)" -ForegroundColor Green
    Write-Host "  - Multi-LoRA management" -ForegroundColor Green
    Write-Host "  - Async inference engine" -ForegroundColor Green
    Write-Host "  - KV cache optimization (PagedAttention v1.4.0)" -ForegroundColor Green
} else {
    Write-Host "✗ Build failed" -ForegroundColor Red
}

Pop-Location
Pop-Location
exit $result
