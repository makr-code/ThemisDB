#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Quick Docker Build & Push for ThemisDB
.DESCRIPTION
    Simplified script for building and pushing ThemisDB Docker images to Docker Hub.
    Designed for release workflows and automated deployment.
.EXAMPLE
    .\quick-docker-deploy.ps1
    .\quick-docker-deploy.ps1 -Tag "1.3.4" -Push
    .\quick-docker-deploy.ps1 -Dockerfile "Dockerfile.simple" -BuildBinary
#>

param(
    [string]$Tag = (Get-Content -Path (Join-Path (Split-Path -Parent $PSScriptRoot) "VERSION") -ErrorAction SilentlyContinue | Select-Object -First 1).Trim(),
    [string]$Dockerfile = "Dockerfile.simple",
    [string]$Platforms = "linux/amd64",
    [switch]$Push,
    [switch]$BuildBinary,
    [switch]$NoCache
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not $Tag) {
    Write-Host "ERROR: VERSION file not found. Set -Tag parameter or ensure VERSION file exists." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "╔═══════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  Quick Docker Deploy for ThemisDB v$Tag   ║" -ForegroundColor Cyan
Write-Host "╚═══════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

$buildArgs = @(
    "-Platforms", $Platforms
    "-Tag", $Tag
    "-Dockerfile", $Dockerfile
)

if ($Push) { $buildArgs += "-Push" }
if ($BuildBinary) { $buildArgs += "-BuildBinary" }
if ($NoCache) { $buildArgs += "-NoCache" }

Write-Host "Invoking Docker build script..." -ForegroundColor Cyan
& "$scriptDir\build-docker.ps1" @buildArgs

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "[OK] Docker deployment completed successfully!" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "[ERROR] Docker deployment failed!" -ForegroundColor Red
    exit 1
}
