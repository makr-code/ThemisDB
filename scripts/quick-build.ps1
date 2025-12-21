#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Quick Build - Cache Update + Standardbuild
.DESCRIPTION
    Schnellster Weg zum Bauen: Cache aktualisieren und dann bauen
    
    Äquivalent zu: .\scripts\build.ps1 -Target windows
#>

Write-Host "╔════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║    ThemisDB Quick Build                ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════╝" -ForegroundColor Green

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$scriptPath = Join-Path $scriptDir "build.ps1"

if (-not (Test-Path $scriptPath)) {
    Write-Host "`n✗ Error: Build-Skript nicht gefunden" -ForegroundColor Red
    Write-Host "  Expected: $scriptPath" -ForegroundColor Red
    exit 1
}

Write-Host "`nStarte Build mit automatischer Cache-Aktualisierung..." -ForegroundColor Cyan
& $scriptPath -Target windows

exit $LASTEXITCODE
