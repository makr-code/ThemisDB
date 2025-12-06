#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Unified Multi-Arch Docker Build for ThemisDB (Docker Desktop / Offline)
    
.DESCRIPTION
    Builds all Docker image variants for ThemisDB using Docker Desktop with buildx.
    This script is designed for local/offline builds without requiring internet access
    during the build process (vcpkg dependencies are cached).
    
    Supported Build Variants:
    - Standard (Ubuntu 22.04): linux/amd64, linux/arm64
    - QNAP (Ubuntu 20.04): linux/amd64 (GLIBC 2.31 compatibility)
    - Raspberry Pi (ARM64): linux/arm64
    
.PARAMETER Version
    Version tag for the images (default: from VERSION file or 1.0.0)
    
.PARAMETER Registry
    Docker registry prefix (default: themisdb)
    
.PARAMETER Variant
    Build variant: 'all', 'standard', 'qnap', 'rpi' (default: all)
    
.PARAMETER Platform
    Override platform for standard build (default: linux/amd64,linux/arm64)
    
.PARAMETER Push
    Push images to registry after build (requires docker login)
    
.PARAMETER NoBuildCache
    Disable Docker build cache
    
.EXAMPLE
    # Build all variants locally
    .\docker-build-multiarch.ps1
    
.EXAMPLE
    # Build only QNAP variant
    .\docker-build-multiarch.ps1 -Variant qnap
    
.EXAMPLE
    # Build and push to registry
    .\docker-build-multiarch.ps1 -Push
    
.EXAMPLE
    # Build specific version without cache
    .\docker-build-multiarch.ps1 -Version 1.0.1 -NoBuildCache
#>

param(
    [string]$Version = '',
    [string]$Registry = 'themisdb',
    [ValidateSet('all', 'standard', 'qnap', 'rpi')]
    [string]$Variant = 'all',
    [string]$Platform = '',
    [switch]$Push = $false,
    [switch]$NoBuildCache = $false
)

$ErrorActionPreference = 'Stop'

# =============================================================================
# Configuration
# =============================================================================

# Read version from VERSION file if not specified
if (-not $Version) {
    $versionFile = Join-Path $PSScriptRoot 'VERSION'
    if (Test-Path $versionFile) {
        $Version = (Get-Content $versionFile -Raw).Trim()
    } else {
        $Version = '1.0.0'
    }
}

# Builder name for buildx
$BuilderName = 'themis-multiarch'

# Build results tracking
$BuildResults = @{}

# =============================================================================
# Helper Functions
# =============================================================================

function Write-Header {
    param([string]$Text)
    Write-Host ''
    Write-Host ('=' * 70) -ForegroundColor Blue
    Write-Host " $Text" -ForegroundColor Blue
    Write-Host ('=' * 70) -ForegroundColor Blue
    Write-Host ''
}

function Write-Step {
    param([string]$Text)
    Write-Host "► $Text" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Text)
    Write-Host "✓ $Text" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Text)
    Write-Host "⚠ $Text" -ForegroundColor Yellow
}

function Write-Failure {
    param([string]$Text)
    Write-Host "✗ $Text" -ForegroundColor Red
}

function Test-DockerBuildx {
    Write-Step 'Checking Docker buildx...'
    
    $buildxVersion = docker buildx version 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Failure 'Docker buildx is not available'
        Write-Host 'Please ensure Docker Desktop is installed with buildx support.' -ForegroundColor Yellow
        return $false
    }
    
    Write-Success "Docker buildx available: $($buildxVersion -split "`n" | Select-Object -First 1)"
    return $true
}

function Initialize-MultiArchBuilder {
    Write-Step "Initializing multi-arch builder: $BuilderName"
    
    $existingBuilder = docker buildx ls 2>&1 | Select-String $BuilderName
    
    if (-not $existingBuilder) {
        Write-Host '  Creating new builder instance...' -ForegroundColor Gray
        docker buildx create --name $BuilderName --use --platform linux/amd64,linux/arm64,linux/arm/v7
        if ($LASTEXITCODE -ne 0) {
            Write-Failure 'Failed to create buildx builder'
            return $false
        }
    } else {
        Write-Host '  Using existing builder instance...' -ForegroundColor Gray
        docker buildx use $BuilderName
    }
    
    # Bootstrap the builder
    docker buildx inspect --bootstrap 2>&1 | Out-Null
    
    Write-Success 'Multi-arch builder ready'
    return $true
}

function Build-DockerImage {
    param(
        [string]$Name,
        [string]$Dockerfile,
        [string]$Platforms,
        [string[]]$Tags,
        [hashtable]$BuildArgs = @{}
    )
    
    Write-Header "Building: $Name"
    
    Write-Host "  Dockerfile: $Dockerfile" -ForegroundColor Gray
    Write-Host "  Platforms:  $Platforms" -ForegroundColor Gray
    Write-Host "  Tags:" -ForegroundColor Gray
    foreach ($tag in $Tags) {
        Write-Host "    - $tag" -ForegroundColor White
    }
    Write-Host ''
    
    # Build command arguments
    $cmdArgs = @('buildx', 'build')
    
    # Add cache options
    if ($NoBuildCache) {
        $cmdArgs += '--no-cache'
    }
    
    # Add platform
    $cmdArgs += '--platform'
    $cmdArgs += $Platforms
    
    # Add build arguments
    foreach ($key in $BuildArgs.Keys) {
        $cmdArgs += '--build-arg'
        $cmdArgs += "$key=$($BuildArgs[$key])"
    }
    
    # Add tags
    foreach ($tag in $Tags) {
        $cmdArgs += '-t'
        $cmdArgs += $tag
    }
    
    # Add dockerfile
    $cmdArgs += '-f'
    $cmdArgs += $Dockerfile
    
    # Progress output
    $cmdArgs += '--progress'
    $cmdArgs += 'plain'
    
    # Push or load
    if ($Push) {
        $cmdArgs += '--push'
    } else {
        # For multi-platform builds without push, we can only use --load for single platform
        if ($Platforms -notmatch ',') {
            $cmdArgs += '--load'
        } else {
            # Multi-platform without push - build to local cache only
            Write-Warning 'Multi-platform build without push - images will be in buildx cache only'
            Write-Host '  Use -Push to push to registry, or build single platform with -Platform' -ForegroundColor Gray
        }
    }
    
    # Context
    $cmdArgs += '.'
    
    # Execute build
    Write-Step 'Starting build...'
    $startTime = Get-Date
    
    & docker @cmdArgs
    $buildExitCode = $LASTEXITCODE
    
    $duration = (Get-Date) - $startTime
    
    if ($buildExitCode -eq 0) {
        Write-Success "Build completed in $($duration.ToString('mm\:ss'))"
        return $true
    } else {
        Write-Failure "Build failed with exit code $buildExitCode"
        return $false
    }
}

# =============================================================================
# Build Definitions
# =============================================================================

function Build-Standard {
    $platforms = if ($Platform) { $Platform } else { 'linux/amd64,linux/arm64' }
    
    $tags = @(
        "$Registry/themisdb:$Version",
        "$Registry/themisdb:latest"
    )
    
    $buildArgs = @{
        'QNAP_BUILD' = 'OFF'
    }
    
    return Build-DockerImage `
        -Name 'Standard Multi-Arch (Ubuntu 22.04)' `
        -Dockerfile 'Dockerfile' `
        -Platforms $platforms `
        -Tags $tags `
        -BuildArgs $buildArgs
}

function Build-QNAP {
    $tags = @(
        "$Registry/themisdb:$Version-qnap",
        "$Registry/themisdb:qnap"
    )
    
    $buildArgs = @{
        'VCPKG_TRIPLET' = 'x64-linux'
    }
    
    return Build-DockerImage `
        -Name 'QNAP (Ubuntu 20.04, x64)' `
        -Dockerfile 'Dockerfile.qnap' `
        -Platforms 'linux/amd64' `
        -Tags $tags `
        -BuildArgs $buildArgs
}

function Build-RaspberryPi {
    $tags = @(
        "$Registry/themisdb:$Version-rpi",
        "$Registry/themisdb:rpi"
    )
    
    $buildArgs = @{
        'TARGETARCH' = 'arm64'
        'VCPKG_TRIPLET' = 'arm64-linux'
        'QNAP_BUILD' = 'OFF'
    }
    
    return Build-DockerImage `
        -Name 'Raspberry Pi (ARM64)' `
        -Dockerfile 'Dockerfile' `
        -Platforms 'linux/arm64' `
        -Tags $tags `
        -BuildArgs $buildArgs
}

# =============================================================================
# Main Execution
# =============================================================================

Write-Header 'ThemisDB Multi-Arch Docker Build'

Write-Host "Version:   $Version" -ForegroundColor White
Write-Host "Registry:  $Registry" -ForegroundColor White
Write-Host "Variant:   $Variant" -ForegroundColor White
Write-Host "Push:      $Push" -ForegroundColor White
Write-Host "No Cache:  $NoBuildCache" -ForegroundColor White

# Check prerequisites
if (-not (Test-DockerBuildx)) {
    exit 1
}

if (-not (Initialize-MultiArchBuilder)) {
    exit 1
}

# Execute builds based on variant selection
$startTime = Get-Date

switch ($Variant) {
    'all' {
        $BuildResults['standard'] = Build-Standard
        $BuildResults['qnap'] = Build-QNAP
        $BuildResults['rpi'] = Build-RaspberryPi
    }
    'standard' {
        $BuildResults['standard'] = Build-Standard
    }
    'qnap' {
        $BuildResults['qnap'] = Build-QNAP
    }
    'rpi' {
        $BuildResults['rpi'] = Build-RaspberryPi
    }
}

$totalDuration = (Get-Date) - $startTime

# =============================================================================
# Summary
# =============================================================================

Write-Header 'Build Summary'

$successCount = ($BuildResults.Values | Where-Object { $_ -eq $true }).Count
$failCount = ($BuildResults.Values | Where-Object { $_ -eq $false }).Count

Write-Host "Total Duration: $($totalDuration.ToString('hh\:mm\:ss'))" -ForegroundColor White
Write-Host ''

Write-Host 'Build Results:' -ForegroundColor Cyan
foreach ($key in $BuildResults.Keys) {
    $status = if ($BuildResults[$key]) { '✓ Success' } else { '✗ Failed' }
    $color = if ($BuildResults[$key]) { 'Green' } else { 'Red' }
    Write-Host "  $key : $status" -ForegroundColor $color
}

Write-Host ''

if ($failCount -eq 0) {
    Write-Success "All $successCount build(s) completed successfully!"
    
    Write-Host ''
    Write-Host 'Built Images:' -ForegroundColor Cyan
    
    if ($BuildResults.ContainsKey('standard')) {
        Write-Host "  $Registry/themisdb:$Version (amd64, arm64)" -ForegroundColor White
        Write-Host "  $Registry/themisdb:latest" -ForegroundColor White
    }
    if ($BuildResults.ContainsKey('qnap')) {
        Write-Host "  $Registry/themisdb:$Version-qnap (amd64)" -ForegroundColor White
        Write-Host "  $Registry/themisdb:qnap" -ForegroundColor White
    }
    if ($BuildResults.ContainsKey('rpi')) {
        Write-Host "  $Registry/themisdb:$Version-rpi (arm64)" -ForegroundColor White
        Write-Host "  $Registry/themisdb:rpi" -ForegroundColor White
    }
    
    if (-not $Push) {
        Write-Host ''
        Write-Host 'To push images to registry:' -ForegroundColor Yellow
        Write-Host "  .\docker-build-multiarch.ps1 -Variant $Variant -Push" -ForegroundColor Gray
    }
    
    Write-Host ''
    Write-Host 'To test locally:' -ForegroundColor Yellow
    Write-Host "  docker run --rm -p 18765:18765 $Registry/themisdb:latest" -ForegroundColor Gray
    
    exit 0
} else {
    Write-Failure "$failCount of $($successCount + $failCount) build(s) failed"
    exit 1
}
