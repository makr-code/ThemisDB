#!/usr/bin/env pwsh
# Rebuild ThemisDB Docker Image for QNAP deployment
# Ensures compatibility with older GLIBC/GLIBCXX by using Ubuntu 20.04 base

param(
    [string]$Tag = "latest",
    [switch]$Push = $false,
    [string]$Registry = ""
)

Write-Host "==> Building ThemisDB for QNAP (x64-linux, Ubuntu 20.04 base)" -ForegroundColor Cyan

# Build multi-stage image
docker build `
    --build-arg VCPKG_TRIPLET=x64-linux `
    -t "themis:$Tag" `
    -f Dockerfile `
    .

if ($LASTEXITCODE -ne 0) {
    Write-Error "Docker build failed"
    exit 1
}

Write-Host "==> Build successful: themis:$Tag" -ForegroundColor Green

if ($Push -and $Registry) {
    $RemoteTag = "${Registry}/themis:${Tag}"
    Write-Host "==> Tagging for registry: $RemoteTag" -ForegroundColor Cyan
    docker tag "themis:$Tag" $RemoteTag
    
    Write-Host "==> Pushing to registry..." -ForegroundColor Cyan
    docker push $RemoteTag
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "==> Push successful: $RemoteTag" -ForegroundColor Green
    } else {
        Write-Error "Push failed"
        exit 1
    }
}

Write-Host ""
Write-Host "==> Next steps for QNAP deployment:" -ForegroundColor Yellow
Write-Host "1. Save image: docker save themis:$Tag | gzip > themis-$Tag.tar.gz"
Write-Host "2. Transfer to QNAP via SCP/SFTP"
Write-Host "3. Load on QNAP: docker load < themis-$Tag.tar.gz"
Write-Host "4. Deploy with: docker-compose -f docker-compose.qnap.yml up -d"
Write-Host ""
