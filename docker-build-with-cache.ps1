#!/usr/bin/env pwsh
# ============================================================================
# Docker Build Script with vcpkg Triple-Cache Strategy
# ============================================================================
# This script builds ThemisDB Docker image using:
# 1. BuildKit cache mounts (persistent between builds)
# 2. Host downloads bind-mount (4.42 GB local cache)
# 3. vcpkg binary cache (compiled packages as .zip)

param(
    [string]$Edition = "COMMUNITY",
    [string]$Tag = "themisdb:latest",
    [string]$LogFile = "docker-build-cached.log",
    [switch]$NoBuildCache
)

$ErrorActionPreference = "Stop"

Write-Host "╔══════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  ThemisDB Docker Build - Triple Cache Strategy          ║" -ForegroundColor Cyan
Write-Host "╚══════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

# Check if vcpkg/downloads exists (required for cache)
$vcpkgDownloads = "vcpkg\downloads"
$hasDownloadsCache = Test-Path $vcpkgDownloads

if ($hasDownloadsCache) {
    $downloadFiles = Get-ChildItem $vcpkgDownloads -File -Recurse
    $downloadSize = ($downloadFiles | Measure-Object -Property Length -Sum).Sum / 1GB
    $downloadCount = $downloadFiles.Count
    
    Write-Host "✓ vcpkg downloads Cache gefunden" -ForegroundColor Green
    Write-Host "  Pfad:    $vcpkgDownloads" -ForegroundColor Gray
    Write-Host "  Dateien: $downloadCount" -ForegroundColor Gray
    Write-Host "  Größe:   $([Math]::Round($downloadSize, 2)) GB" -ForegroundColor Gray
    Write-Host "  Strategie: Bind-Mount (keine Kopie, direkter Zugriff)" -ForegroundColor Gray
} else {
    Write-Host "⚠ Kein vcpkg downloads Cache gefunden" -ForegroundColor Yellow
    Write-Host "  Pfad: $vcpkgDownloads" -ForegroundColor Gray
    Write-Host "  Auswirkung: Downloads von Internet (langsamer)" -ForegroundColor Gray
}

# Validate Docker BuildKit
if (-not $env:DOCKER_BUILDKIT) {
    Write-Host ""
    Write-Host "✓ Aktiviere Docker BuildKit..." -ForegroundColor Yellow
    $env:DOCKER_BUILDKIT = 1
}

# Create minimal build context (exclude vcpkg - we mount it)
Write-Host ""
Write-Host "📦 Erstelle minimalen Build Context..." -ForegroundColor Cyan
$tempContext = ".docker-build-minimal"

if (Test-Path $tempContext) {
    Remove-Item $tempContext -Recurse -Force
}

mkdir $tempContext | Out-Null

# Copy essential files only
$items = @(
    "Dockerfile",
    "CMakeLists.txt",
    "VERSION",
    "RELEASE_TYPE",
    "vcpkg.json",
    "vcpkg-configuration.json",
    "cmake",
    "include",
    "src",
    "proto",
    "internal",
    "adapters",
    "aql",
    "docker",
    "llama.cpp",
    "ports",
    "tests"
)

$copiedCount = 0
foreach ($item in $items) {
    if (Test-Path $item) {
        Copy-Item $item "$tempContext\" -Recurse -Force
        $copiedCount++
    }
}

# Copy vcpkg/downloads to enable cache
if ($hasDownloadsCache) {
    Write-Host "  Kopiere vcpkg downloads..." -ForegroundColor Gray
    $targetDownloads = "$tempContext\vcpkg\downloads"
    mkdir $targetDownloads -Force | Out-Null
    
    # Copy only archives (skip tools subdirs)
    Get-ChildItem $vcpkgDownloads -File -Recurse | Where-Object {
        $_.Extension -in @('.tar.gz', '.tar.bz2', '.tar.xz', '.zip', '.7z', '.tgz') -or
        $_.Name -like '*LICENSE*'
    } | ForEach-Object {
        Copy-Item $_.FullName $targetDownloads -Force -ErrorAction SilentlyContinue
    }
    
    $copiedDownloads = (Get-ChildItem $targetDownloads -File).Count
    Write-Host "  ✓ $copiedDownloads downloads kopiert" -ForegroundColor Green
}

$contextSize = (Get-ChildItem $tempContext -Recurse -File | Measure-Object -Property Length -Sum).Sum / 1MB
Write-Host "✓ Context: $copiedCount items, $([Math]::Round($contextSize, 2)) MB" -ForegroundColor Green

# Build with Triple Cache Strategy
Write-Host ""
Write-Host "🐳 Starte Docker Build..." -ForegroundColor Cyan
Write-Host "──────────────────────────────────────────────────────────" -ForegroundColor DarkGray
Write-Host "  Edition:     $Edition" -ForegroundColor Gray
Write-Host "  Tag:         $Tag" -ForegroundColor Gray
Write-Host "  Log:         $LogFile" -ForegroundColor Gray
Write-Host "  Cache:       Triple-Cache-Strategie" -ForegroundColor Gray
Write-Host "    1. BuildKit cache mounts (persistent)" -ForegroundColor DarkGray
Write-Host "    2. Host downloads COPY in Build Context" -ForegroundColor DarkGray
Write-Host "    3. vcpkg binary cache (.zip archives)" -ForegroundColor DarkGray

$buildArgs = @(
    "buildx", "build"
)

# Add build arguments
$buildArgs += "--build-arg", "THEMIS_EDITION=$Edition"
$buildArgs += "--build-arg", "ENABLE_LLM=ON"
$buildArgs += "--build-arg", "ENABLE_GPU=OFF"
$buildArgs += "--progress=plain"
$buildArgs += "-t", $Tag

# Disable Docker build cache if requested
if ($NoBuildCache) {
    $buildArgs += "--no-cache"
    Write-Host "  Build Cache: DISABLED (--no-cache)" -ForegroundColor Yellow
} else {
    Write-Host "  Build Cache: ENABLED" -ForegroundColor Green
}

# Add context path
$buildArgs += "-f", "Dockerfile"
$buildArgs += "."

Write-Host ""
Write-Host "Befehl: docker $($buildArgs -join ' ')" -ForegroundColor DarkGray
Write-Host ""
Write-Host "Build läuft... (30-45 Min beim 1. Build, 2-5 Min danach)" -ForegroundColor Cyan
Write-Host "Abbrechen: Ctrl+C" -ForegroundColor Gray
Write-Host ""

# Execute build from temp context
Push-Location $tempContext
try {
    & docker @buildArgs 2>&1 | Tee-Object -FilePath "..\$LogFile"
    $buildResult = $LASTEXITCODE
}
finally {
    Pop-Location
}

Write-Host ""

if ($buildResult -eq 0) {
    Write-Host "╔══════════════════════════════════════════════════════════╗" -ForegroundColor Green
    Write-Host "║            ✓ Build erfolgreich!                         ║" -ForegroundColor Green
    Write-Host "╚══════════════════════════════════════════════════════════╝" -ForegroundColor Green
    Write-Host ""
    Write-Host "Image Information:" -ForegroundColor Cyan
    docker images $Tag --format "  {{.Repository}}:{{.Tag}}  {{.Size}}  (erstellt {{.CreatedAt}})"
    Write-Host ""
    Write-Host "Nächste Schritte:" -ForegroundColor Yellow
    Write-Host "  # Container starten" -ForegroundColor Gray
    Write-Host "  docker run -p 9001:9001 $Tag" -ForegroundColor White
    Write-Host ""
    Write-Host "  # Version prüfen" -ForegroundColor Gray
    Write-Host "  docker run --rm $Tag themis_server --version" -ForegroundColor White
    Write-Host ""
    Write-Host "  # CLI-Hilfe" -ForegroundColor Gray
    Write-Host "  docker run --rm $Tag themis_server --help" -ForegroundColor White
} else {
    Write-Host "╔══════════════════════════════════════════════════════════╗" -ForegroundColor Red
    Write-Host "║            ❌ Build fehlgeschlagen!                      ║" -ForegroundColor Red
    Write-Host "╚══════════════════════════════════════════════════════════╝" -ForegroundColor Red
    Write-Host ""
    Write-Host "Exit Code: $buildResult" -ForegroundColor Yellow
    Write-Host "Log-Datei: $LogFile" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Letzte Fehler:" -ForegroundColor Yellow
    Get-Content $LogFile -ErrorAction SilentlyContinue | Select-String "error:" -Context 2,1 | Select-Object -Last 5
    Write-Host ""
    Write-Host "Vollständiges Log anzeigen:" -ForegroundColor Gray
    Write-Host "  Get-Content $LogFile | Select-String 'error:|FAILED:' -Context 5,2" -ForegroundColor White
}

exit $buildResult
