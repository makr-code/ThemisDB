<#
.SYNOPSIS
    Build ThemisDB for QNAP (Ubuntu 20.04, GLIBC 2.31)

.DESCRIPTION
    Statischer Build Ã¼ber einen Ubuntu 20.04 Container. Verzicht auf Presets
    (direkte CMake Aufrufe) und Abschalten optionaler Komponenten fÃ¼r
    maximalen Erfolg bei Ã¤lterer Toolchain.

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

Write-Host "Building static ThemisDB binary (direct configure, QNAP manifest)..." -ForegroundColor Yellow

# Compose bash command without Windows CRLF issues
$envBlock = "export VCPKG_MANIFEST_DIR=/src && export VCPKG_FEATURE_FLAGS=manifests"
$configure = (
    "cmake -S . -B build-qnap -G Ninja" +
    " -DCMAKE_BUILD_TYPE=Release" +
    " -DTHEMIS_STATIC_BUILD=ON" +
    " -DTHEMIS_BUILD_TESTS=ON" +
    " -DTHEMIS_BUILD_BENCHMARKS=ON" +
    " -DTHEMIS_ENABLE_TRACING=ON" +
    " -DVCPKG_MANIFEST_FILE=vcpkg.qnap.json" +
    " -DVCPKG_TARGET_TRIPLET=x64-linux-release" +
    " -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake" +
    " -DOPENSSL_ROOT_DIR=/usr" +
    " -DOPENSSL_INCLUDE_DIR=/usr/include" +
    " -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/x86_64-linux-gnu/libcrypto.so" +
    " -DOPENSSL_SSL_LIBRARY=/usr/lib/x86_64-linux-gnu/libssl.so"
)
$build = "cmake --build build-qnap --parallel"
$verify = "ls -l build-qnap | grep themis_server || echo 'Binary fehlt noch'"

$bashCmd = "set -e; echo '>>> Ninja Version:'; ninja --version; echo '>>> vcpkg bootstrap (ensure up to date)'; /opt/vcpkg/bootstrap-vcpkg.sh; echo '>>> vcpkg manifest install (x64-linux-release triplet)'; cd /src; /opt/vcpkg/vcpkg install --triplet x64-linux-release --x-manifest-root=/src || { echo 'vcpkg install failed'; exit 1; }; echo '>>> Configure'; $envBlock && $configure; echo '>>> Build'; $build; echo '>>> Verify'; $verify"

& docker run --rm -v "${PWD}:/src" -w /src themisdb-qnap-builder:latest bash -lc "$bashCmd"

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
