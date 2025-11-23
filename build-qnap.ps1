<#
.SYNOPSIS
    Build ThemisDB for QNAP (Ubuntu 20.04, GLIBC 2.31)

.DESCRIPTION
    Builds a statically-linked ThemisDB binary compatible with QNAP NAS
    using Ubuntu 20.04 build container for GLIBC 2.31 compatibility.

.EXAMPLE
    .\build-qnap.ps1
#>

param(
    [switch]$NoBuildContainer,
    [switch]$NoCache
)

$ErrorActionPreference = 'Stop'

Write-Host "=== ThemisDB QNAP Build (Ubuntu 20.04, GLIBC 2.31) ===" -ForegroundColor Cyan
Write-Host ""

# Check Docker availability
$dockerAvailable = Get-Command docker -ErrorAction SilentlyContinue
if (-not $dockerAvailable) {
    Write-Host "ERROR: Docker not found! Install Docker Desktop." -ForegroundColor Red
    exit 1
}

# Build build container if needed
if (-not $NoBuildContainer) {
    Write-Host "Building themisdb-qnap-builder container..." -ForegroundColor Yellow
    
    $dockerArgs = @(
        'build',
        '-f', 'Dockerfile.qnap.build',
        '-t', 'themisdb-qnap-builder:latest',
        '.'
    )
    
    if ($NoCache) {
        $dockerArgs += '--no-cache'
    }
    
    & docker $dockerArgs
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to build container!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Build container ready!" -ForegroundColor Green
    Write-Host ""
}

# Build static binary in container
Write-Host "Building static ThemisDB binary..." -ForegroundColor Yellow

$buildCommand = @"
cmake --preset linux-ninja-gcc-release -DTHEMIS_STATIC_BUILD=ON
cmake --build --preset linux-ninja-gcc-release
"@

& docker run --rm `
    -v "${PWD}:/src" `
    -w /src `
    themisdb-qnap-builder:latest `
    bash -c $buildCommand

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "=== Build Successful! ===" -ForegroundColor Green
Write-Host ""
Write-Host "Binary location:" -ForegroundColor Cyan
Write-Host "  build-linux-gcc-release/themis_server" -ForegroundColor White
Write-Host ""
Write-Host "Verify GLIBC compatibility:" -ForegroundColor Cyan
Write-Host "  docker run --rm -v `"`${PWD}:/src`" ubuntu:20.04 ldd /src/build-linux-gcc-release/themis_server | grep GLIBC" -ForegroundColor Gray
Write-Host ""
Write-Host "Deploy to QNAP:" -ForegroundColor Cyan
Write-Host "  1. Copy binary to QNAP via SCP/SFTP" -ForegroundColor Gray
Write-Host "  2. Make executable: chmod +x themis_server" -ForegroundColor Gray
Write-Host "  3. Run: ./themis_server --config config.yml" -ForegroundColor Gray
Write-Host ""
