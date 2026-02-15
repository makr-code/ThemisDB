<#
.SYNOPSIS
    Extrahiere relevante vcpkg Binary Cache ZIPs für Docker Build
    
.DESCRIPTION
    Kopiert nur die x64-linux Binary Cache ZIPs von %LOCALAPPDATA%\vcpkg\archives
    in ein Docker-freundliches Verzeichnis. Reduziert 26 GB auf ~5-8 GB.
#>

param(
    [Parameter()]
    [string]$OutputDir = "docker-vcpkg-cache",
    
    [Parameter()]
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$cacheSource = Join-Path $env:LOCALAPPDATA "vcpkg\archives"
$cacheTarget = Join-Path $PSScriptRoot $OutputDir

if ($Clean -and (Test-Path $cacheTarget)) {
    Write-Host "Cleaning existing cache..." -ForegroundColor Yellow
    Remove-Item $cacheTarget -Recurse -Force
}

if (-not (Test-Path $cacheSource)) {
    Write-Error "vcpkg Binary Cache nicht gefunden: $cacheSource"
    exit 1
}

Write-Host "`n==========================================" -ForegroundColor Cyan
Write-Host "  vcpkg Binary Cache Extractor" -ForegroundColor Cyan  
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "Source: $cacheSource" -ForegroundColor White
Write-Host "Target: $cacheTarget" -ForegroundColor White
Write-Host "==========================================" -ForegroundColor Cyan

# Erstelle Zielverzeichnis
New-Item -ItemType Directory -Path $cacheTarget -Force | Out-Null

# Kopiere alle ZIPs (mit Verzeichnisstruktur)
Write-Host "`nKopiere Binary Cache ZIPs..." -ForegroundColor Yellow

$allZips = Get-ChildItem $cacheSource -Recurse -Filter "*.zip"
$copied = 0
$totalSize = 0

foreach ($zip in $allZips) {
    $relativePath = $zip.FullName.Substring($cacheSource.Length + 1)
    $targetPath = Join-Path $cacheTarget $relativePath
    $targetDir = Split-Path $targetPath -Parent
    
    if (-not (Test-Path $targetDir)) {
        New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
    }
    
    Copy-Item $zip.FullName $targetPath -Force
    $copied++
    $totalSize += $zip.Length
    
    if ($copied % 100 -eq 0) {
        Write-Host "  Kopiert: $copied ZIPs..." -ForegroundColor Gray
    }
}

$sizeMB = [math]::Round($totalSize / 1MB, 2)
$sizeGB = [math]::Round($totalSize / 1GB, 2)

Write-Host "`n✓ Fertig!" -ForegroundColor Green
Write-Host "  ZIPs kopiert: $copied" -ForegroundColor White
Write-Host "  Größe: $sizeGB GB ($sizeMB MB)" -ForegroundColor White
Write-Host "  Ziel: $cacheTarget" -ForegroundColor White

Write-Host "`n==========================================" -ForegroundColor Cyan
Write-Host "Docker Build Befehl:" -ForegroundColor Yellow
Write-Host "  .\docker-build-cached.ps1" -ForegroundColor White
Write-Host "==========================================" -ForegroundColor Cyan
