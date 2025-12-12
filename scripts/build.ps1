#!/usr/bin/env pwsh
<#
.SYNOPSIS
    ThemisDB Build Orchestrator - Updates cache then runs specified build
.DESCRIPTION
    Provides unified build interface for Windows, Linux, and Docker builds
    with automatic vcpkg cache updates
.EXAMPLE
    .\build.ps1 -Target windows
    .\build.ps1 -Target docker -Platforms "linux/amd64,linux/arm64" -Push
    .\build.ps1 -Target all
#>

param(
    [ValidateSet("windows", "linux", "docker", "all")]
    [string]$Target = "windows",
    [string]$Platforms = "linux/amd64,linux/arm64",
    [switch]$Push,
    [switch]$NoCache,
    [string]$Tag = ((Get-Content -Path (Join-Path $PSScriptRoot "..\VERSION") -ErrorAction SilentlyContinue) | Select-Object -First 1).Trim()
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

Write-Host "==============================" -ForegroundColor Green
Write-Host "ThemisDB Build Orchestrator v1.0" -ForegroundColor Green
Write-Host "==============================" -ForegroundColor Green

Write-Host "`nTarget: $Target" -ForegroundColor Cyan
Write-Host "Cache Update: $(if ($NoCache) { 'DISABLED' } else { 'ENABLED' })" -ForegroundColor Cyan

function Build-Windows {
    Write-Host "`n════════ Windows Build (MSVC) ════════" -ForegroundColor Yellow
    & "$scriptDir\build-windows.ps1" -NoCache:$NoCache
    if ($LASTEXITCODE -ne 0) { throw "Windows build failed" }
    & "$scriptDir\collect-artifacts.ps1"
}

function Build-Linux {
    Write-Host "`n════════ Linux Build (GCC/Ninja) ════════" -ForegroundColor Yellow
    & "$scriptDir\build-linux.sh"
    if ($LASTEXITCODE -ne 0) { throw "Linux build failed" }
    & "$scriptDir\collect-artifacts.ps1"
}

function Build-Docker {
    Write-Host "`n════════ Docker Build (Multi-Arch) ════════" -ForegroundColor Yellow
    if (-not $Tag) { throw "Version konnte nicht aus VERSION gelesen werden. Bitte VERSION pflegen oder -Tag angeben." }
    $dockerArgs = @(
        "-Platforms", $Platforms
        "-Tag", $Tag
        "-NoCache:$NoCache"
    )
    if ($Push) { $dockerArgs += "-Push" }
    & "$scriptDir\build-docker.ps1" @dockerArgs
    if ($LASTEXITCODE -ne 0) { throw "Docker build failed" }
}

try {
    switch ($Target) {
        "windows" { Build-Windows }
        "linux" { Build-Linux }
        "docker" { Build-Docker }
        "all" {
            Build-Windows
            Build-Linux
            Build-Docker
        }
    }
    
Write-Host "`n==============================" -ForegroundColor Green
Write-Host "Build Successful!" -ForegroundColor Green
Write-Host "==============================" -ForegroundColor Green
    # Nach erfolgreichem Build stets Checksummen erstellen (Security Validation)
    $releaseDir = Join-Path (Split-Path -Parent $scriptDir) "release"
    & "$scriptDir\package-checksums.ps1" -ArtifactsDir $releaseDir -OutputFile (Join-Path $releaseDir "SHA256SUMS.txt")
}
catch {
    Write-Host "`n==============================" -ForegroundColor Red
    Write-Host "Build Failed!" -ForegroundColor Red
    Write-Host "$_" -ForegroundColor Red
    Write-Host "==============================" -ForegroundColor Red
    exit 1
}
