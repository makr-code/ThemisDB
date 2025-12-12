#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build ThemisDB Docker image(s) with automatic cache update
.DESCRIPTION
    Updates vcpkg cache, then builds Docker image(s) with buildx
.EXAMPLE
    .\build-docker.ps1
    .\build-docker.ps1 -Platforms "linux/amd64"
    .\build-docker.ps1 -Platforms "linux/amd64,linux/arm64" -Push
#>

param(
    [string]$Platforms = "linux/amd64,linux/arm64",
    [switch]$Push,
    [switch]$NoCache,
    [string]$Tag = ((Get-Content -Path (Join-Path $PSScriptRoot "..\VERSION") -ErrorAction SilentlyContinue) | Select-Object -First 1).Trim(),
    [string]$Registry = "themisdb"
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Split-Path -Parent $scriptDir

Write-Host "=== ThemisDB Docker Build ===" -ForegroundColor Cyan
Write-Host "Platforms: $Platforms" -ForegroundColor Yellow
if (-not $Tag) { throw "Version konnte nicht aus VERSION gelesen werden. Bitte VERSION pflegen oder -Tag angeben." }
Write-Host "Tag: $Registry/themisdb:$Tag" -ForegroundColor Yellow
Write-Host "Push: $Push" -ForegroundColor Yellow

# Step 1: Update cache
if (-not $NoCache) {
    Write-Host "`n[1/2] Updating vcpkg cache (offline sources)..." -ForegroundColor Cyan
    & "$scriptDir\update-vcpkg-cache.ps1" -Triplets @("x64-linux", "arm64-linux")
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Cache update failed" -ForegroundColor Red
        exit 1
    }
}

# Step 2: Build with Docker buildx
Write-Host "`n[2/2] Building Docker image(s)..." -ForegroundColor Cyan
$buildxArgs = @(
    "buildx", "build"
    "--builder", "themis-multiarch"
    "--platform", $Platforms
    "-f", "Dockerfile"
    "-t", "$Registry/themisdb:$Tag"
    "-t", "$Registry/themisdb:latest"
    "--build-arg", "THEMIS_VERSION=$Tag"
)

if ($Push) {
    $buildxArgs += "--push"
}
else {
    $buildxArgs += "--load"
}

# Add no-cache if requested
if ($NoCache) {
    $buildxArgs += "--no-cache"
}

$buildxArgs += "."
$buildxArgs += "--progress=plain"

Write-Host "Running: docker $($buildxArgs -join ' ')" -ForegroundColor DarkGray
Push-Location $rootDir
try {
    & docker @buildxArgs
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Docker build failed" -ForegroundColor Red
        exit 1
    }

    # Wenn gepusht, Digest ermitteln und als Sicherheitsreferenz speichern
    if ($Push) {
        $img = "$Registry/themisdb:$Tag"
        Write-Host "Ermittle Image-Digest für $img ..." -ForegroundColor Cyan
        $inspect = & docker buildx imagetools inspect $img 2>$null
        if ($LASTEXITCODE -eq 0 -and $inspect) {
            $digest = ($inspect | Select-String -Pattern "sha256:[0-9a-f]{64}" -AllMatches | Select-Object -First 1).Matches.Value
            $releaseDir = Join-Path $rootDir "release"
            New-Item -ItemType Directory -Path $releaseDir -Force | Out-Null
            $outFile = Join-Path $releaseDir ("docker-digest-$Tag.txt")
            if ($digest) {
                "Image: $img`nDigest: $digest" | Out-File -FilePath $outFile -Encoding ASCII
                Write-Host "✓ Docker Digest gespeichert: $outFile" -ForegroundColor Green
            } else {
                "Inspect Output:`n$inspect" | Out-File -FilePath $outFile -Encoding ASCII
                Write-Host "⚠ Digest nicht gefunden, Inspect-Output gespeichert: $outFile" -ForegroundColor Yellow
            }
        }
    }
}
finally {
    Pop-Location
}

Write-Host "`n=== Docker Build Complete ===" -ForegroundColor Green
Write-Host "Image: $Registry/themisdb:$Tag" -ForegroundColor Green
if ($Push) {
    Write-Host "Status: Pushed to registry" -ForegroundColor Green
}
else {
    Write-Host "Status: Loaded locally (use --push to push to registry)" -ForegroundColor Yellow
}
