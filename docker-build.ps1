#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Docker Build for ThemisDB (Hybrid Pre-built Binary)
    
.DESCRIPTION
    Builds Docker images for ThemisDB using pre-built binaries.
    Uses standard 'docker build' (no buildx required).
    
    This hybrid approach provides:
    - Fast build times (seconds instead of minutes)
    - Small image sizes (~100-200 MB)
    - 100% offline capability
    
    Workflow:
    1. Build themis_server binary locally (WSL/Linux) with vcpkg
    2. Copy binary to build/ directory
    3. Create Docker image with Dockerfile.simple
    
.PARAMETER Version
    Version tag for the images (default: from VERSION file or 1.0.0)
    
.PARAMETER Registry
    Docker registry prefix (default: themisdb)
    
.PARAMETER Variant
    Build variant: 'standard', 'qnap' (default: standard)
    
.PARAMETER BinaryPath
    Path to pre-built themis_server binary (default: build/themis_server)
    
.PARAMETER BuildBinary
    Build binary in WSL before creating Docker image
    
.PARAMETER Push
    Push images to registry after build (requires docker login)
    
.PARAMETER NoBuildCache
    Disable Docker build cache
    
.EXAMPLE
    # Build Docker image with existing binary
    .\docker-build.ps1
    
.EXAMPLE
    # Build binary in WSL first, then create Docker image
    .\docker-build.ps1 -BuildBinary
    
.EXAMPLE
    # Build QNAP variant
    .\docker-build.ps1 -Variant qnap
    
.EXAMPLE
    # Build and push to registry
    .\docker-build.ps1 -Push
#>

param(
    [string]$Version = '',
    [string]$Registry = 'themisdb',
    [ValidateSet('standard', 'qnap')]
    [string]$Variant = 'standard',
    [string]$BinaryPath = 'build/themis_server',
    [switch]$BuildBinary = $false,
    [switch]$Push = $false,
    [switch]$NoBuildCache = $false
)

$ErrorActionPreference = 'Stop'

# =============================================================================
# Configuration
# =============================================================================

if (-not $Version) {
    $versionFile = Join-Path $PSScriptRoot 'VERSION'
    if (Test-Path $versionFile) {
        $Version = (Get-Content $versionFile -Raw).Trim()
    } else {
        $Version = '1.0.0'
    }
}

# =============================================================================
# Helper Functions
# =============================================================================

function Write-Header {
    param([string]$Text)
    Write-Host ''
    Write-Host ('=' * 60) -ForegroundColor Blue
    Write-Host " $Text" -ForegroundColor Blue
    Write-Host ('=' * 60) -ForegroundColor Blue
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

# =============================================================================
# Build Binary in WSL (optional)
# =============================================================================

function Build-BinaryInWSL {
    Write-Header 'Building Binary in WSL'
    
    Write-Step 'Checking WSL availability...'
    $wslCheck = wsl --list 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Failure 'WSL is not available'
        return $false
    }
    Write-Success 'WSL is available'
    
    Write-Step 'Checking for existing build directory...'
    $buildExists = wsl bash -lc "test -d ~/themis-build-release && echo 'exists' || echo 'missing'"
    
    if ($buildExists -match 'missing') {
        Write-Warning 'Build directory not found. Please run setup first:'
        Write-Host '  wsl bash -lc "mkdir -p ~/themis-build-release && cd ~/themis-build-release && cmake -S /path/to/ThemisDB -B . -DCMAKE_BUILD_TYPE=Release"' -ForegroundColor Gray
        return $false
    }
    
    Write-Step 'Building themis_server in WSL...'
    wsl bash -lc "cd ~/themis-build-release && cmake --build . --target themis_server -j`$(nproc)"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Failure 'Build failed in WSL'
        return $false
    }
    
    Write-Step 'Copying binary to Windows...'
    $buildDir = Join-Path $PSScriptRoot 'build'
    if (-not (Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
    }
    
    $wslPath = (wsl wslpath -u (Get-Location).Path) -replace '\s+$', ''
    wsl bash -lc "cp ~/themis-build-release/themis_server '$wslPath/build/'"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Failure 'Failed to copy binary'
        return $false
    }
    
    Write-Success 'Binary built and copied successfully'
    return $true
}

# =============================================================================
# Docker Build
# =============================================================================

function Build-DockerImage {
    param(
        [string]$Dockerfile,
        [string[]]$Tags
    )
    
    Write-Header "Building Docker Image"
    
    Write-Host "  Dockerfile: $Dockerfile" -ForegroundColor Gray
    Write-Host "  Binary:     $BinaryPath" -ForegroundColor Gray
    Write-Host "  Tags:" -ForegroundColor Gray
    foreach ($tag in $Tags) {
        Write-Host "    - $tag" -ForegroundColor White
    }
    Write-Host ''
    
    if (-not (Test-Path $BinaryPath)) {
        Write-Failure "Binary not found at: $BinaryPath"
        Write-Host ''
        Write-Host 'Please build the binary first:' -ForegroundColor Yellow
        Write-Host '  Option 1: .\docker-build.ps1 -BuildBinary' -ForegroundColor Gray
        Write-Host '  Option 2: Build manually in WSL and copy to build/' -ForegroundColor Gray
        return $false
    }
    
    Write-Success "Binary found: $BinaryPath"
    
    # Build command
    $cmdArgs = @('build')
    
    if ($NoBuildCache) {
        $cmdArgs += '--no-cache'
    }
    
    foreach ($tag in $Tags) {
        $cmdArgs += '-t'
        $cmdArgs += $tag
    }
    
    $cmdArgs += '-f'
    $cmdArgs += $Dockerfile
    $cmdArgs += '.'
    
    Write-Step 'Starting Docker build...'
    $startTime = Get-Date
    
    & docker @cmdArgs
    $buildExitCode = $LASTEXITCODE
    
    $duration = (Get-Date) - $startTime
    
    if ($buildExitCode -eq 0) {
        Write-Success "Build completed in $($duration.ToString('mm\:ss'))"
        
        if ($Push) {
            Write-Step 'Pushing images to registry...'
            foreach ($tag in $Tags) {
                Write-Host "  Pushing: $tag" -ForegroundColor Gray
                docker push $tag
                if ($LASTEXITCODE -ne 0) {
                    Write-Failure "Failed to push: $tag"
                    return $false
                }
            }
            Write-Success 'All images pushed successfully'
        }
        
        return $true
    } else {
        Write-Failure "Build failed with exit code $buildExitCode"
        return $false
    }
}

# =============================================================================
# Main Execution
# =============================================================================

Write-Header 'ThemisDB Docker Build (Hybrid Pre-built Binary)'

Write-Host "Version:      $Version" -ForegroundColor White
Write-Host "Registry:     $Registry" -ForegroundColor White
Write-Host "Variant:      $Variant" -ForegroundColor White
Write-Host "Binary Path:  $BinaryPath" -ForegroundColor White
Write-Host "Build Binary: $BuildBinary" -ForegroundColor White
Write-Host "Push:         $Push" -ForegroundColor White

Write-Step 'Checking Docker...'
$dockerVersion = docker version --format '{{.Server.Version}}' 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Failure 'Docker is not running'
    exit 1
}
Write-Success "Docker available: $dockerVersion"

if ($BuildBinary) {
    if (-not (Build-BinaryInWSL)) {
        exit 1
    }
}

# Determine tags based on variant
$tags = @()
$dockerfile = 'Dockerfile.simple'

switch ($Variant) {
    'standard' {
        $tags = @(
            "$Registry/themisdb:$Version",
            "$Registry/themisdb:latest"
        )
    }
    'qnap' {
        $tags = @(
            "$Registry/themisdb:$Version-qnap",
            "$Registry/themisdb:qnap"
        )
    }
}

$startTime = Get-Date
$success = Build-DockerImage -Dockerfile $dockerfile -Tags $tags
$totalDuration = (Get-Date) - $startTime

# =============================================================================
# Summary
# =============================================================================

Write-Header 'Build Summary'

Write-Host "Total Duration: $($totalDuration.ToString('mm\:ss'))" -ForegroundColor White
Write-Host ''

if ($success) {
    Write-Success 'Build completed successfully!'
    
    Write-Host ''
    Write-Host 'Built Images:' -ForegroundColor Cyan
    foreach ($tag in $tags) {
        Write-Host "  $tag" -ForegroundColor White
    }
    
    Write-Host ''
    Write-Host 'To test locally:' -ForegroundColor Yellow
    Write-Host "  docker run --rm -p 18765:18765 $($tags[0])" -ForegroundColor Gray
    
    Write-Host ''
    Write-Host 'To run with data volume:' -ForegroundColor Yellow
    Write-Host "  docker run -d -p 18765:18765 -v `${PWD}/data:/data $($tags[0])" -ForegroundColor Gray
    
    if (-not $Push) {
        Write-Host ''
        Write-Host 'To push to registry:' -ForegroundColor Yellow
        Write-Host "  .\docker-build.ps1 -Variant $Variant -Push" -ForegroundColor Gray
    }
    
    exit 0
} else {
    Write-Failure 'Build failed'
    exit 1
}
