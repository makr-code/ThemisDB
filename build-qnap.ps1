<#
.SYNOPSIS
    Build ThemisDB for QNAP (Ubuntu 20.04, GLIBC 2.31)

.DESCRIPTION
    Statischer Build über einen Ubuntu 20.04 Container. Verzicht auf Presets
    (direkte CMake Aufrufe) und Abschalten optionaler Komponenten für
    maximalen Erfolg bei älterer Toolchain.

.EXAMPLE
    .\build-qnap.ps1
    .\build-qnap.ps1 -NoCache
    .\build-qnap.ps1 -NoBuildContainer
#>

param(
    [switch]$NoBuildContainer,
    [switch]$NoCache
)

$ErrorActionPreference = 'Stop'

Write-Host "=== ThemisDB QNAP Build (Ubuntu 20.04, GLIBC 2.31) ===" -ForegroundColor Cyan
Write-Host ""

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: Docker not found!" -ForegroundColor Red
    exit 1
}

if (-not $NoBuildContainer) {
    Write-Host "Building themisdb-qnap-builder container..." -ForegroundColor Yellow
    $dockerArgs = @('build','-f','Dockerfile.qnap.build','-t','themisdb-qnap-builder:latest','.')
    if ($NoCache) { $dockerArgs += '--no-cache' }
    & docker $dockerArgs
    if ($LASTEXITCODE -ne 0) { Write-Host "ERROR: Failed to build container" -ForegroundColor Red; exit 1 }
    Write-Host "Container ready." -ForegroundColor Green
    Write-Host ""
}

Write-Host "Building static ThemisDB binary (direct configure)..." -ForegroundColor Yellow

$cmakeConfigure = @"
cmake -S . -B build-qnap \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_STATIC_BUILD=ON \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF \
  -DTHEMIS_ENABLE_TRACING=OFF \
  -DVCPKG_TARGET_TRIPLET=x64-linux-static
"@

$cmakeBuild = "cmake --build build-qnap --parallel"

$fullBuildScript = @"
set -e
echo '>>> Ninja Version:'
ninja --version || { echo 'Ninja fehlt!'; exit 1; }
echo '>>> Configure'
$cmakeConfigure
echo '>>> Build'
$cmakeBuild
"@

& docker run --rm -v "${PWD}:/src" -w /src themisdb-qnap-builder:latest bash -lc $fullBuildScript

if ($LASTEXITCODE -ne 0) { Write-Host "ERROR: Build failed!" -ForegroundColor Red; exit 1 }

Write-Host ""; Write-Host "=== Build Successful! ===" -ForegroundColor Green; Write-Host ""
Write-Host "Binary location:" -ForegroundColor Cyan
Write-Host "  build-qnap/themis_server" -ForegroundColor White
Write-Host ""
Write-Host "Verify GLIBC compatibility:" -ForegroundColor Cyan
Write-Host "  docker run --rm -v \"`${PWD}:/src\" ubuntu:20.04 ldd /src/build-qnap/themis_server | grep GLIBC" -ForegroundColor Gray
Write-Host ""
Write-Host "Deploy to QNAP:" -ForegroundColor Cyan
Write-Host "  1. Copy binary to QNAP via SCP/SFTP" -ForegroundColor Gray
Write-Host "  2. chmod +x themis_server" -ForegroundColor Gray
Write-Host "  3. ./themis_server --config config.yml" -ForegroundColor Gray
Write-Host ""
