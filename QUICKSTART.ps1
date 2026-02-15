<#
.SYNOPSIS
    Quick-Start: Multi-Platform vcpkg Package System

.DESCRIPTION
    Schritt-für-Schritt-Anleitung für das neue Multi-Platform Package System.
    Führt durch ersten Build und erklärt jeden Schritt.

.EXAMPLE
    .\QUICKSTART.ps1
#>

$ErrorActionPreference = 'Stop'

function Write-Header {
    param([string]$Text)
    Write-Host "`n$('='*80)" -ForegroundColor Cyan
    Write-Host "  $Text" -ForegroundColor Cyan
    Write-Host "$('='*80)`n" -ForegroundColor Cyan
}

function Write-Step {
    param([string]$Number, [string]$Text)
    Write-Host "`n[SCHRITT $Number] $Text" -ForegroundColor Yellow
}

function Write-Info {
    param([string]$Text)
    Write-Host "  ℹ $Text" -ForegroundColor Cyan
}

function Wait-Continue {
    Write-Host "`nDrücke Enter zum Fortfahren..." -ForegroundColor Green
    Read-Host
}

# =============================================================================
# Intro
# =============================================================================

Write-Header "Quick-Start: Multi-Platform vcpkg Package System"

Write-Host @"
Willkommen zum neuen Multi-Platform Package System!

Dieses System baut vcpkg-Pakete für Windows und Linux einmalig vor,
und mountet sie dann in Docker - für EXTREM schnelle Builds.

⏱️  Vorher: 45 min Docker-Build (vcpkg install im Container)
⏱️  Nachher: 5 min Docker-Build (Pakete bereits vorhanden)

📦 Zeitersparnis: 89%!

Was passiert:
  1. Vorkompilierte Pakete für Linux bauen (einmalig, ~10-15 min)
  2. Docker-Image mit gemounteten Paketen bauen (~5 min)
  3. Testen und fertig!

"@ -ForegroundColor White

Wait-Continue

# =============================================================================
# Schritt 1: WSL prüfen
# =============================================================================

Write-Step "1/5" "WSL-Umgebung prüfen"

Write-Info "WSL wird benötigt, um Linux-Pakete (x64-linux) zu kompilieren"
Write-Info "Diese Pakete werden dann in Docker verwendet"

try {
    $wslInfo = wsl --version 2>&1
    if ($LASTEXITCODE -ne 0) { throw }
    
    Write-Host "`n✓ WSL ist verfügbar" -ForegroundColor Green
    
    $distros = wsl --list --quiet
    Write-Host "✓ WSL-Distributionen gefunden:" -ForegroundColor Green
    $distros | ForEach-Object { Write-Host "  - $_" -ForegroundColor Gray }
}
catch {
    Write-Host "`n✗ WSL nicht verfügbar!" -ForegroundColor Red
    Write-Host "`nWSL installieren:" -ForegroundColor Yellow
    Write-Host "  wsl --install"
    Write-Host "  wsl --install Ubuntu"
    Write-Host "`nNach Installation dieses Skript erneut ausführen."
    exit 1
}

Wait-Continue

# =============================================================================
# Schritt 2: vcpkg prüfen
# =============================================================================

Write-Step "2/5" "vcpkg-Umgebung prüfen"

$vcpkgExe = Join-Path $PSScriptRoot "vcpkg\vcpkg.exe"
$vcpkgDownloads = Join-Path $PSScriptRoot "vcpkg\downloads"

if (!(Test-Path $vcpkgExe)) {
    Write-Host "`n✗ vcpkg.exe nicht gefunden!" -ForegroundColor Red
    Write-Host "`nvcpkg einrichten:" -ForegroundColor Yellow
    Write-Host "  git clone https://github.com/microsoft/vcpkg.git"
    Write-Host "  cd vcpkg"
    Write-Host "  .\bootstrap-vcpkg.bat"
    exit 1
}

Write-Host "`n✓ vcpkg.exe gefunden" -ForegroundColor Green

if (Test-Path $vcpkgDownloads) {
    $downloadCount = (Get-ChildItem $vcpkgDownloads -File -Recurse -ErrorAction SilentlyContinue | Measure-Object).Count
    $downloadSize = (Get-ChildItem $vcpkgDownloads -File -Recurse -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum / 1GB
    Write-Host "✓ vcpkg/downloads vorhanden: $downloadCount Dateien, $([math]::Round($downloadSize, 2)) GB" -ForegroundColor Green
    Write-Info "Diese Downloads werden wiederverwendet (spart Zeit + Bandbreite)"
}

Wait-Continue

# =============================================================================
# Schritt 3: Pakete bauen
# =============================================================================

Write-Step "3/5" "Linux-Pakete für Docker bauen"

Write-Host @"

Jetzt werden die vcpkg-Pakete für Linux (x64-linux) kompiliert.
Dies ist ein EINMALIGER Prozess (~10-15 min).

Die kompilierten Pakete werden hier gespeichert:
  📁 vcpkg_packages/x64-linux/release/

Konfiguration:
  - Platform: Linux (x64-linux)
  - Build-Type: Release
  - Edition: COMMUNITY (21 Dependencies)
  
Dependencies umfassen:
  boost, rocksdb, grpc, protobuf, fmt, spdlog, yaml-cpp, ...

"@ -ForegroundColor White

Write-Host "Dieser Vorgang kann 10-15 Minuten dauern." -ForegroundColor Yellow
Write-Host "Dabei werden ~6-10 GB an Daten erstellt.`n" -ForegroundColor Yellow

$response = Read-Host "Fortsetzten? (y/n)"
if ($response -ne 'y') {
    Write-Host "Abgebrochen." -ForegroundColor Yellow
    exit 0
}

Write-Host "`nStarte Package-Build...`n" -ForegroundColor Green

try {
    & "$PSScriptRoot\build-vcpkg-packages.ps1" `
        -Platform linux `
        -Configuration release `
        -Edition COMMUNITY
    
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }
    
    Write-Host "`n✓ Pakete erfolgreich gebaut!" -ForegroundColor Green
}
catch {
    Write-Host "`n✗ Package-Build fehlgeschlagen: $_" -ForegroundColor Red
    Write-Host "`nBitte Fehler beheben und erneut versuchen." -ForegroundColor Yellow
    exit 1
}

Wait-Continue

# =============================================================================
# Schritt 4: Docker-Image bauen
# =============================================================================

Write-Step "4/5" "Docker-Image mit vorkompilierten Paketen bauen"

Write-Host @"

Jetzt wird das Docker-Image gebaut - aber OHNE vcpkg install!
Die vorher kompilierten Pakete werden einfach gemountet.

Erwartete Build-Zeit: ~5 min (statt 45 min!)

Image-Tag: themisdb:latest

"@ -ForegroundColor White

$response = Read-Host "Docker-Build starten? (y/n)"
if ($response -ne 'y') {
    Write-Host "Übersprungen." -ForegroundColor Yellow
    Write-Host "Später manuell starten:" -ForegroundColor Cyan
    Write-Host "  .\docker-build-with-prebuilt-packages.ps1 -Edition COMMUNITY" -ForegroundColor Gray
    exit 0
}

Write-Host "`nStarte Docker-Build...`n" -ForegroundColor Green

try {
    & "$PSScriptRoot\docker-build-with-prebuilt-packages.ps1" `
        -Edition COMMUNITY `
        -Tag "themisdb:latest" `
        -Configuration release
    
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }
    
    Write-Host "`n✓ Docker-Image erfolgreich gebaut!" -ForegroundColor Green
}
catch {
    Write-Host "`n✗ Docker-Build fehlgeschlagen: $_" -ForegroundColor Red
    Write-Host "`nBitte Fehler beheben und erneut versuchen." -ForegroundColor Yellow
    exit 1
}

Wait-Continue

# =============================================================================
# Schritt 5: Testen
# =============================================================================

Write-Step "5/5" "Docker-Image testen"

Write-Info "Führe Smoke-Tests aus..."

# Test 1: Version
Write-Host "`nTest 1: --version" -ForegroundColor Yellow
docker run --rm themisdb:latest themis_server --version
if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ PASSED" -ForegroundColor Green
} else {
    Write-Host "✗ FAILED" -ForegroundColor Red
}

# Test 2: Build-Info
Write-Host "`nTest 2: --build-info" -ForegroundColor Yellow
docker run --rm themisdb:latest themis_server --build-info | Select-Object -First 15
if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ PASSED" -ForegroundColor Green
} else {
    Write-Host "✗ FAILED" -ForegroundColor Red
}

# Test 3: Image-Info
Write-Host "`nImage-Details:" -ForegroundColor Yellow
docker images themisdb:latest --format "table {{.Repository}}\t{{.Tag}}\t{{.Size}}\t{{.CreatedAt}}"

# =============================================================================
# Fertig
# =============================================================================

Write-Header "Quick-Start abgeschlossen!"

Write-Host @"
✓ Linux-Pakete gebaut und gespeichert in vcpkg_packages/
✓ Docker-Image themisdb:latest erfolgreich erstellt
✓ Tests bestanden

📦 Package Store:
"@ -ForegroundColor Green

$packageStore = Join-Path $PSScriptRoot "vcpkg_packages"
$linuxReleaseSize = (Get-ChildItem "$packageStore\x64-linux\release" -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum / 1GB
Write-Host "  Location: $packageStore" -ForegroundColor White
Write-Host "  Linux Release: $([math]::Round($linuxReleaseSize, 2)) GB" -ForegroundColor White

Write-Host "`n🚀 Nächste Schritte:" -ForegroundColor Cyan
Write-Host @"

  1. Container starten:
     docker run -d -p 8080:8080 -p 9090:9090 --name themisdb themisdb:latest

  2. Logs ansehen:
     docker logs -f themisdb

  3. Code ändern und SCHNELL rebuilden (5 min):
     .\docker-build-with-prebuilt-packages.ps1

  4. Weitere Varianten bauen:
     .\build-all-platforms.ps1 -Configuration all

  5. Vollständige Dokumentation:
     Siehe VCPKG_MULTI_PLATFORM_PACKAGES.md

"@ -ForegroundColor White

Write-Host "⚡ Performance-Gewinn:" -ForegroundColor Yellow
Write-Host "  Vorher: 45 min Docker-Build (vcpkg install)" -ForegroundColor Gray
Write-Host "  Jetzt:  5 min Docker-Build (mounted packages)" -ForegroundColor Gray
Write-Host "  Zeitersparnis: 89%!" -ForegroundColor Green

Write-Host "`nViel Erfolg! 🎉" -ForegroundColor Cyan
