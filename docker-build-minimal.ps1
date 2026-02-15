#!/usr/bin/env pwsh
# ============================================================================
# Docker Build Script with minimal build context (no vcpkg scan)
# ============================================================================

param(
    [string]$Edition = "COMMUNITY",
    [string]$Tag = "themisdb:latest",
    [switch]$NoBuildCache,
    [switch]$UseVcpkgCache = $true
)

$ErrorActionPreference = "Stop"

Write-Host "╔══════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  ThemisDB Docker Build (Minimal Context)                ║" -ForegroundColor Cyan
Write-Host "╚══════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

# Create temporary build context
$buildContext = ".docker-build-tmp"
Write-Host "Creating minimal build context..." -ForegroundColor Cyan

if (Test-Path $buildContext) {
    Remove-Item $buildContext -Recurse -Force
}
New-Item -ItemType Directory -Path $buildContext | Out-Null

# Copy essential files and directories
$itemsToCopy = @(
    "CMakeLists.txt",
    "VERSION",
    "vcpkg.json",
    "vcpkg-configuration.json",
    "Dockerfile",
    "cmake",
    "include",
    "src",
    "proto",
    "internal",
    "adapters",
    "aql",
    "docker",
    "llama.cpp",
    "ports"
)

Write-Host "Copying source files..." -ForegroundColor Gray
foreach ($item in $itemsToCopy) {
    if (Test-Path $item) {
        Copy-Item $item -Destination "$buildContext\$item" -Recurse -Force
        Write-Host "  ✓ $item" -ForegroundColor DarkGray
    }
}

# Create minimal .dockerignore
@"
# Minimal .dockerignore - only exclude build artifacts in copied dirs
**/build/**
**/build-*/**
llama.cpp/build/**
llama.cpp/.git/**
"@ | Out-File "$buildContext\.dockerignore" -Encoding utf8

Write-Host "✓ Build context ready" -ForegroundColor Green

# Check vcpkg cache
$vcpkgPackagesPath = ".\vcpkg\packages"
$hasVcpkgCache = Test-Path $vcpkgPackagesPath

if ($hasVcpkgCache) {
    $packageCount = (Get-ChildItem $vcpkgPackagesPath -Directory -Filter "*_x64-linux" | Measure-Object).Count
    Write-Host "✓ Found $packageCount x64-linux packages in vcpkg cache" -ForegroundColor Green
} else {
    Write-Host "⚠ No vcpkg cache found" -ForegroundColor Yellow
    $UseVcpkgCache = $false
}

# Build Docker image
Write-Host ""
Write-Host "Building Docker image..." -ForegroundColor Cyan
Write-Host "──────────────────────────────────────────────────────────" -ForegroundColor DarkGray
Write-Host "  Edition: $Edition" -ForegroundColor Gray
Write-Host "  Tag: $Tag" -ForegroundColor Gray
Write-Host "  vcpkg Cache: $UseVcpkgCache" -ForegroundColor Gray
Write-Host ""

$buildArgs = @(
    "buildx", "build"
)

# Add vcpkg cache context
if ($UseVcpkgCache -and $hasVcpkgCache) {
    $buildArgs += "--build-context", "prebuilt=$vcpkgPackagesPath"
    $buildArgs += "--build-arg", "ENABLE_VCPKG_CACHE=ON"
}

# Add other build arguments
$buildArgs += "--build-arg", "THEMIS_EDITION=$Edition"
$buildArgs += "--build-arg", "ENABLE_LLM=ON"
$buildArgs += "--build-arg", "ENABLE_GPU=OFF"
$buildArgs += "-t", $Tag
$buildArgs += "-f", "$buildContext\Dockerfile"

if ($NoBuildCache) {
    $buildArgs += "--no-cache"
}

# Build context is the temp directory
$buildArgs += $buildContext

Write-Host "Executing: docker $buildArgs" -ForegroundColor DarkGray
Write-Host ""

try {
    & docker @buildArgs
    $buildResult = $LASTEXITCODE
}
finally {
    # Cleanup
    Write-Host ""
    Write-Host "Cleaning up build context..." -ForegroundColor Yellow
    Remove-Item $buildContext -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "✓ Cleanup complete" -ForegroundColor Green
}

if ($buildResult -ne 0) {
    Write-Host ""
    Write-Host "✗ Docker build failed" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "╔══════════════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║  ✓ Build completed successfully!                        ║" -ForegroundColor Green
Write-Host "╚══════════════════════════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""
Write-Host "Image: $Tag" -ForegroundColor Cyan
Write-Host ""
Write-Host "Run with: docker run -p 9001:9001 $Tag" -ForegroundColor Gray
