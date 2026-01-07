#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Deploy ThemisDB Docker Image to Docker Hub
.DESCRIPTION
    Builds Docker image and pushes to Docker Hub registry.
.EXAMPLE
    .\deploy-docker.ps1 -Tag "1.3.4" -Push
#>

param(
    [string]$Tag = (Get-Content -Path (Join-Path (Split-Path -Parent $PSScriptRoot) "VERSION") -ErrorAction SilentlyContinue | Select-Object -First 1).Trim(),
    [string]$Dockerfile = "Dockerfile.simple",
    [switch]$Push
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not $Tag) {
    Write-Host "ERROR: VERSION file not found" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Docker Deploy for ThemisDB v$Tag" -ForegroundColor Cyan
Write-Host ""

if (-not (docker --version 2>$null)) {
    Write-Host "[ERROR] Docker not installed or not running" -ForegroundColor Red
    exit 1
}

# Check if binary exists for simple Dockerfile
if ($Dockerfile -eq "Dockerfile.simple") {
    $binaryFile = Join-Path (Split-Path -Parent $scriptDir) "release/themis_server.exe"
    if (-not (Test-Path $binaryFile)) {
        Write-Host "[ERROR] Binary not found: $binaryFile" -ForegroundColor Red
        Write-Host "Build it first with: .\build-docker.ps1 -BuildBinary -Tag $Tag" -ForegroundColor Yellow
        exit 1
    }
    Write-Host "[OK] Binary found: $binaryFile" -ForegroundColor Green
}

# Build Docker image
Write-Host ""
Write-Host "Building Docker image..." -ForegroundColor Cyan
$repoRoot = Split-Path -Parent $scriptDir
$imageTag = "themisdb/themis:$Tag"
$imageTagLatest = "themisdb/themis:latest"

$dockerArgs = @(
    "build",
    "-f", (Join-Path $repoRoot "docker/$Dockerfile"),
    "-t", $imageTag,
    "-t", $imageTagLatest,
    "--build-arg", "VERSION=$Tag"
)

& docker @dockerArgs (Join-Path $repoRoot "docker")
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Docker build failed" -ForegroundColor Red
    exit 1
}

Write-Host "[OK] Image built: $imageTag" -ForegroundColor Green

# Push to Docker Hub if requested
if ($Push) {
    Write-Host ""
    Write-Host "Pushing to Docker Hub..." -ForegroundColor Cyan
    
    # Check Docker login
    $loginStatus = docker info 2>$null | Select-String "Username"
    if (-not $loginStatus) {
        Write-Host "Logging in to Docker Hub..." -ForegroundColor Cyan
        & docker login
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[ERROR] Docker login failed" -ForegroundColor Red
            exit 1
        }
    }
    
    # Push both tags
    Write-Host "Pushing $imageTag..." -ForegroundColor Cyan
    & docker push $imageTag
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Push failed for $imageTag" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Pushing $imageTagLatest..." -ForegroundColor Cyan
    & docker push $imageTagLatest
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Push failed for $imageTagLatest" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "[OK] Images pushed successfully!" -ForegroundColor Green
}

Write-Host ""
if ($Push) {
    Write-Host "[SUCCESS] Docker image deployed to Docker Hub!" -ForegroundColor Green
    Write-Host "Pull with: docker pull $imageTag" -ForegroundColor Cyan
} else {
    Write-Host "[SUCCESS] Docker image built successfully!" -ForegroundColor Green
    Write-Host "Push with: docker push $imageTag" -ForegroundColor Cyan
}
Write-Host ""
