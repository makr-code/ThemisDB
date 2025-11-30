#!/usr/bin/env pwsh
# Build and push QNAP Docker images with Ubuntu 20.04 for compatibility
# Usage: .\build-docker-qnap.ps1 [-Tag <tag>] [-Push]

param(
    [string]$Tag = "themisdb/themisdb:qnap",
    [switch]$Push = $false
)

$ErrorActionPreference = "Stop"

Write-Host "=== ThemisDB QNAP Docker Build ===" -ForegroundColor Cyan
Write-Host "Building with Ubuntu 20.04 for GLIBC 2.31 compatibility" -ForegroundColor Yellow
Write-Host ""

# Build multi-arch images for QNAP (x86_64 and ARM models)
$tags = @(
    "themisdb/themisdb:qnap",
    "themisdb/themisdb:1.0.0-qnap"
)

$tagArgs = $tags | ForEach-Object { "-t", $_ }

Write-Host "Building QNAP Docker image (x64)..." -ForegroundColor Green
docker build -f Dockerfile.qnap `
    --build-arg VCPKG_TRIPLET=x64-linux `
    @tagArgs `
    .

if ($LASTEXITCODE -ne 0) {
    Write-Error "Docker build failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "✓ Build successful!" -ForegroundColor Green
Write-Host ""
Write-Host "Built images:" -ForegroundColor Cyan
$tags | ForEach-Object { Write-Host "  - $_" -ForegroundColor White }

if ($Push) {
    Write-Host ""
    Write-Host "Pushing images to DockerHub..." -ForegroundColor Green
    
    foreach ($tag in $tags) {
        Write-Host "  Pushing $tag..." -ForegroundColor Yellow
        docker push $tag
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Failed to push $tag"
            exit $LASTEXITCODE
        }
    }
    
    Write-Host ""
    Write-Host "✓ All images pushed successfully!" -ForegroundColor Green
}

Write-Host ""
Write-Host "To test the image:" -ForegroundColor Cyan
Write-Host "  docker run --rm -p 18765:18765 themisdb/themisdb:qnap" -ForegroundColor White
Write-Host ""
Write-Host "To deploy on QNAP:" -ForegroundColor Cyan
Write-Host "  1. Copy docker-compose.qnap.yml to QNAP" -ForegroundColor White
Write-Host "  2. Run: docker-compose -f docker-compose.qnap.yml up -d" -ForegroundColor White
Write-Host ""
