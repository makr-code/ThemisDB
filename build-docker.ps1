#!/usr/bin/env pwsh
# ============================================================================
# ThemisDB Docker Build Script - Windows PowerShell
# ============================================================================
# Builds ThemisDB Docker images with BuildKit caching and best practices
#
# Usage:
#   .\build-docker.ps1                        # Build COMMUNITY edition
#   .\build-docker.ps1 -Edition ENTERPRISE    # Build ENTERPRISE edition
#   .\build-docker.ps1 -EnableLLM -EnableGPU  # Build with LLM+GPU
#   .\build-docker.ps1 -Push                  # Build and push to registry
#   .\build-docker.ps1 -NoBuildKit            # Disable BuildKit
#
# Prerequisites:
#   - Docker Desktop with BuildKit enabled
#   - Git (for version tagging)

[CmdletBinding()]
param(
    [Parameter(HelpMessage="ThemisDB Edition to build")]
    [ValidateSet("MINIMAL", "COMMUNITY", "ENTERPRISE", "HYPERSCALER")]
    [string]$Edition = "COMMUNITY",
    
    [Parameter(HelpMessage="Enable LLM support")]
    [switch]$EnableLLM,
    
    [Parameter(HelpMessage="Enable GPU support (Vulkan/CUDA)")]
    [switch]$EnableGPU,
    
    [Parameter(HelpMessage="Build tests")]
    [switch]$BuildTests,
    
    [Parameter(HelpMessage="Build benchmarks")]
    [switch]$BuildBenchmarks,
    
    [Parameter(HelpMessage="Target platform (linux/amd64, linux/arm64)")]
    [string]$Platform = "linux/amd64",
    
    [Parameter(HelpMessage="Docker image tag")]
    [string]$Tag = "",
    
    [Parameter(HelpMessage="Push to registry after build")]
    [switch]$Push,
    
    [Parameter(HelpMessage="Registry prefix (e.g., ghcr.io/yourorg)")]
    [string]$Registry = "",
    
    [Parameter(HelpMessage="Disable BuildKit")]
    [switch]$NoBuildKit,
    
    [Parameter(HelpMessage="Clean build (no cache)")]
    [switch]$NoCache,
    
    [Parameter(HelpMessage="Build debug image")]
    [switch]$Debug
)

# ============================================================================
# Configuration
# ============================================================================

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Enable Docker BuildKit (unless disabled)
if (-not $NoBuildKit) {
    $env:DOCKER_BUILDKIT = "1"
    $env:COMPOSE_DOCKER_CLI_BUILD = "1"
}

# Get version from VERSION file
$versionFile = Join-Path $PSScriptRoot "VERSION"
if (Test-Path $versionFile) {
    $version = (Get-Content $versionFile -Raw).Trim()
} else {
    $version = "dev"
}

# Generate tag
if (-not $Tag) {
    $editionLower = $Edition.ToLower()
    $Tag = "${editionLower}-${version}"
}

# Image name
$imageName = "themisdb"
if ($Registry) {
    $imageName = "$Registry/$imageName"
}
$fullImageTag = "${imageName}:${Tag}"

# ============================================================================
# Build Arguments
# ============================================================================

$buildArgs = @{
    "THEMIS_EDITION" = $Edition
    "ENABLE_LLM" = if ($EnableLLM) { "ON" } else { "OFF" }
    "ENABLE_GPU" = if ($EnableGPU) { "ON" } else { "OFF" }
    "FORCE_CPU_ONLY" = if ($EnableGPU) { "OFF" } else { "ON" }
    "BUILD_TESTS" = if ($BuildTests) { "ON" } else { "OFF" }
    "BUILD_BENCHMARKS" = if ($BuildBenchmarks) { "ON" } else { "OFF" }
}

# ============================================================================
# Display Configuration
# ============================================================================

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "ThemisDB Docker Build" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Edition:        $Edition" -ForegroundColor Yellow
Write-Host "Version:        $version" -ForegroundColor Yellow
Write-Host "LLM Support:    $(if ($EnableLLM) { 'ON' } else { 'OFF' })" -ForegroundColor Yellow
Write-Host "GPU Support:    $(if ($EnableGPU) { 'ON' } else { 'OFF' })" -ForegroundColor Yellow
Write-Host "Tests:          $(if ($BuildTests) { 'ON' } else { 'OFF' })" -ForegroundColor Yellow
Write-Host "Benchmarks:     $(if ($BuildBenchmarks) { 'ON' } else { 'OFF' })" -ForegroundColor Yellow
Write-Host "Platform:       $Platform" -ForegroundColor Yellow
Write-Host "Image Tag:      $fullImageTag" -ForegroundColor Green
Write-Host "BuildKit:       $(if ($NoBuildKit) { 'Disabled' } else { 'Enabled' })" -ForegroundColor Yellow
Write-Host "Cache:          $(if ($NoCache) { 'Disabled' } else { 'Enabled' })" -ForegroundColor Yellow
Write-Host "Target:         $(if ($Debug) { 'debug' } else { 'runtime' })" -ForegroundColor Yellow
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# ============================================================================
# Build Docker Image
# ============================================================================

Write-Host "Building Docker image..." -ForegroundColor Green

# Construct buildx command
$buildxArgs = @(
    "buildx", "build"
    "--platform", $Platform
    "--target", $(if ($Debug) { "debug" } else { "runtime" })
    "--tag", $fullImageTag
)

# Add build arguments
foreach ($key in $buildArgs.Keys) {
    $buildxArgs += "--build-arg"
    $buildxArgs += "$key=$($buildArgs[$key])"
}

# Add cache options
if (-not $NoCache) {
    if ($Registry) {
        $buildxArgs += "--cache-from", "type=registry,ref=${imageName}:buildcache"
        $buildxArgs += "--cache-to", "type=registry,ref=${imageName}:buildcache,mode=max"
    } else {
        $buildxArgs += "--cache-from", "type=local,src=.docker-cache"
        $buildxArgs += "--cache-to", "type=local,dest=.docker-cache,mode=max"
    }
}

# Load or push image
if ($Push) {
    $buildxArgs += "--push"
} else {
    $buildxArgs += "--load"
}

# Add context (current directory)
$buildxArgs += "."

# Execute build
Write-Host "docker $($buildxArgs -join ' ')" -ForegroundColor DarkGray
& docker @buildxArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "✗ Docker build failed" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "✓ Docker image built successfully: $fullImageTag" -ForegroundColor Green

# ============================================================================
# Additional Tags
# ============================================================================

if (-not $Push) {
    # Tag as 'latest' for local builds
    $latestTag = "${imageName}:latest"
    Write-Host "Tagging as: $latestTag" -ForegroundColor Yellow
    docker tag $fullImageTag $latestTag
    
    # Tag with edition name
    $editionTag = "${imageName}:$($Edition.ToLower())"
    if ($editionTag -ne $fullImageTag) {
        Write-Host "Tagging as: $editionTag" -ForegroundColor Yellow
        docker tag $fullImageTag $editionTag
    }
}

# ============================================================================
# Summary
# ============================================================================

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Build Complete" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Image: $fullImageTag" -ForegroundColor Yellow
Write-Host ""
Write-Host "To run:" -ForegroundColor Cyan
Write-Host "  docker run -p 8080:8080 $fullImageTag" -ForegroundColor White
Write-Host ""
Write-Host "To test:" -ForegroundColor Cyan
Write-Host "  curl http://localhost:8080/health" -ForegroundColor White
Write-Host ""
Write-Host "To start with docker-compose:" -ForegroundColor Cyan
Write-Host "  docker compose up -d" -ForegroundColor White
Write-Host ""
