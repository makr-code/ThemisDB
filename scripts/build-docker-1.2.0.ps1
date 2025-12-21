#!/usr/bin/env pwsh
# Schneller Docker-Build für 1.2.0 mit optimiertem vcpkg-Cache
$ErrorActionPreference = "Stop"

Write-Host "`n=== ThemisDB 1.2.0 Docker Build ===" -ForegroundColor Cyan

# 1. Temporäres vcpkg/downloads vorbereiten
Write-Host "[1/3] Preparing vcpkg downloads..." -ForegroundColor Yellow
$tempDir = New-Item -ItemType Directory -Path ".\vcpkg_downloads_temp" -Force
Copy-Item -Path ".\vcpkg\downloads\*" -Destination $tempDir -Force -Recurse
Write-Host "  ✓ $($(Get-ChildItem $tempDir -File).Count) files ready" -ForegroundColor Green

# 2. Docker Build
Write-Host "[2/3] Building Docker image..." -ForegroundColor Yellow
$version = (Get-Content -First 1 VERSION).Trim()

docker buildx build `
    --builder themis-multiarch `
    --platform linux/amd64 `
    -f Dockerfile `
    -t themisdb/themisdb:$version `
    -t themisdb/themisdb:latest `
    --build-arg THEMIS_VERSION=$version `
    --progress=plain `
    --load `
    .

if ($LASTEXITCODE -ne 0) {
    Write-Host "✗ Build failed!" -ForegroundColor Red
    Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
    exit 1
}

# 3. Cleanup
Write-Host "[3/3] Cleanup..." -ForegroundColor Yellow
Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue
Write-Host "  ✓ Temp files removed" -ForegroundColor Green

Write-Host "`n✓ Build complete: themisdb/themisdb:$version" -ForegroundColor Green
docker images themisdb/themisdb:$version
