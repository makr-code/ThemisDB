<#
.SYNOPSIS
    Build Docker image using pre-built vcpkg packages
    
.DESCRIPTION
    Erstellt Docker-Image mit gemounteten vorkompilierten vcpkg-Paketen aus dem lokalen Package-Store.
    Erfordert vorherige Ausführung von build-vcpkg-packages.ps1.

.PARAMETER Edition
    ThemisDB Edition: COMMUNITY, MINIMAL, ENTERPRISE, HYPERSCALER

.PARAMETER Tag
    Docker Image Tag (default: themisdb:latest)

.PARAMETER Configuration
    Build-Konfiguration: debug oder release (default: release)

.PARAMETER EnableLLM
    LLM-Support aktivieren (default: ON für COMMUNITY/ENTERPRISE/HYPERSCALER)

.PARAMETER EnableGPU
    GPU-Support aktivieren (default: OFF)

.PARAMETER NoBuildCache
    Docker BuildKit Cache deaktivieren

.EXAMPLE
    .\docker-build-with-prebuilt-packages.ps1 -Edition COMMUNITY
    .\docker-build-with-prebuilt-packages.ps1 -Edition ENTERPRISE -Configuration debug -EnableGPU
#>

param(
    [Parameter()]
    [ValidateSet('COMMUNITY', 'MINIMAL', 'ENTERPRISE', 'HYPERSCALER')]
    [string]$Edition = 'COMMUNITY',
    
    [Parameter()]
    [string]$Tag = 'themisdb:latest',
    
    [Parameter()]
    [ValidateSet('debug', 'release')]
    [string]$Configuration = 'release',
    
    [Parameter()]
    [ValidateSet('ON', 'OFF')]
    [string]$EnableLLM = $(if ($Edition -in @('COMMUNITY', 'ENTERPRISE', 'HYPERSCALER')) { 'ON' } else { 'OFF' }),
    
    [Parameter()]
    [ValidateSet('ON', 'OFF')]
    [string]$EnableGPU = 'OFF',
    
    [Parameter()]
    [switch]$NoBuildCache,
    
    [Parameter()]
    [string]$LogFile = "docker-build-prebuilt.log"
)

$ErrorActionPreference = 'Stop'

# =============================================================================
# Configuration
# =============================================================================

$rootDir = $PSScriptRoot
$packageStore = Join-Path $rootDir "vcpkg_packages"
$linuxPackagesDir = Join-Path $packageStore "x64-linux\$Configuration"
$dockerfilePath = Join-Path $rootDir "Dockerfile.prebuilt"

# =============================================================================
# Helper Functions
# =============================================================================

function Write-Header {
    param([string]$Text)
    Write-Host "`n$('='*80)" -ForegroundColor Cyan
    Write-Host "  $Text" -ForegroundColor Cyan
    Write-Host "$('='*80)`n" -ForegroundColor Cyan
}

function Write-Step {
    param([string]$Text)
    Write-Host ">>> $Text" -ForegroundColor Green
}

function Write-Error-Message {
    param([string]$Text)
    Write-Host "ERROR: $Text" -ForegroundColor Red
}

# =============================================================================
# Validation
# =============================================================================

Write-Header "Docker Build with Pre-built vcpkg Packages"

Write-Step "Validating pre-built packages..."

if (!(Test-Path $linuxPackagesDir)) {
    Write-Error-Message "Linux packages not found: $linuxPackagesDir"
    Write-Host "`nRun first: .\build-vcpkg-packages.ps1 -Platform linux -Configuration $Configuration -Edition $Edition"
    exit 1
}

# Größe prüfen
$packageCount = (Get-ChildItem $linuxPackagesDir -Recurse -File -ErrorAction SilentlyContinue | Measure-Object).Count
$packageSize = (Get-ChildItem $linuxPackagesDir -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum / 1MB

if ($packageCount -eq 0) {
    Write-Error-Message "No packages found in $linuxPackagesDir"
    Write-Host "`nRun: .\build-vcpkg-packages.ps1 -Platform linux -Configuration $Configuration -Edition $Edition"
    exit 1
}

Write-Host "  ✓ Found $packageCount files ($([math]::Round($packageSize, 0)) MB)" -ForegroundColor Green

# =============================================================================
# Create Optimized Dockerfile for Pre-built Packages
# =============================================================================

Write-Step "Creating optimized Dockerfile for pre-built packages..."

$dockerfileContent = @"
# syntax=docker/dockerfile:1.6
# ThemisDB Docker Build - Pre-built Packages Strategy
# Uses mounted pre-compiled vcpkg packages from Windows/WSL builds

# =============================================================================
# Base Stage
# =============================================================================
FROM ubuntu:24.04 AS base

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    pkg-config \
    zip \
    unzip \
    tar \
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
# Dependencies Stage - Mount Pre-built Packages
# =============================================================================
FROM base AS deps

# Kopiere nur vcpkg Tool (nicht Pakete)
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git /opt/vcpkg && \
    /opt/vcpkg/bootstrap-vcpkg.sh && \
    rm -rf /opt/vcpkg/.git

# Mount pre-built packages from host
# Diese werden zur Build-Zeit gemounted, nicht in das Image kopiert
COPY vcpkg_packages/x64-linux/$Configuration /opt/vcpkg-prebuilt

# Setze vcpkg-Umgebung für vorhandene Pakete
ENV VCPKG_ROOT=/opt/vcpkg
ENV VCPKG_INSTALLED_DIR=/opt/vcpkg-prebuilt
ENV CMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake

# =============================================================================
# Build Stage
# =============================================================================
FROM deps AS builder

ARG THEMIS_EDITION=COMMUNITY
ARG ENABLE_LLM=ON
ARG ENABLE_GPU=OFF
ARG BUILD_TYPE=Release

# Workspace
WORKDIR /workspace
COPY . /workspace

# llama.cpp submodule
RUN if [ "`$ENABLE_LLM" = "ON" ]; then \
      if [ ! -d llama.cpp/.git ]; then \
        git submodule update --init --recursive --depth 1 llama.cpp; \
      fi; \
    fi

# Build ThemisDB with pre-built dependencies
RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=`$BUILD_TYPE \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_INSTALLED_DIR=/opt/vcpkg-prebuilt \
    -DVCPKG_MANIFEST_MODE=OFF \
    -DTHEMIS_EDITION=`$THEMIS_EDITION \
    -DTHEMIS_BUILD_TESTS=OFF \
    -DTHEMIS_BUILD_BENCHMARKS=OFF \
    -DTHEMIS_ENABLE_LLM=`$ENABLE_LLM \
    -DTHEMIS_ENABLE_GPU=`$ENABLE_GPU \
    && cmake --build build --parallel `$(nproc) --target themis_server

# =============================================================================
# Runtime Stage
# =============================================================================
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Runtime dependencies
RUN apt-get update && apt-get install -y \
    libgomp1 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy binary
COPY --from=builder /workspace/build/src/server/themis_server /usr/local/bin/
COPY --from=builder /workspace/config /etc/themis/config

# Copy runtime libraries from pre-built packages
COPY --from=deps /opt/vcpkg-prebuilt/*/*.so* /usr/local/lib/
RUN ldconfig

# Setup
RUN useradd -r -s /bin/false themis && \
    mkdir -p /var/lib/themis /var/log/themis && \
    chown -R themis:themis /var/lib/themis /var/log/themis

EXPOSE 8080 9090
VOLUME ["/var/lib/themis", "/var/log/themis"]

USER themis
WORKDIR /var/lib/themis

CMD ["themis_server", "--config", "/etc/themis/config/themis.yaml"]
"@

Set-Content -Path $dockerfilePath -Value $dockerfileContent -Encoding UTF8
Write-Host "  ✓ Created $dockerfilePath" -ForegroundColor Green

# =============================================================================
# Create Minimal Build Context
# =============================================================================

Write-Step "Creating minimal build context..."

$buildContextDir = Join-Path $rootDir ".docker-build-prebuilt"
if (Test-Path $buildContextDir) {
    Remove-Item $buildContextDir -Recurse -Force
}
New-Item -Path $buildContextDir -ItemType Directory | Out-Null

# Kopiere essentielle Dateien
$filesToCopy = @(
    "CMakeLists.txt",
    "CMakePresets.json",
    "VERSION",
    "RELEASE_TYPE",
    "src",
    "include",
    "cmake",
    "config",
    "llama.cpp"
)

foreach ($item in $filesToCopy) {
    $source = Join-Path $rootDir $item
    $dest = Join-Path $buildContextDir $item
    if (Test-Path $source) {
        if (Test-Path $source -PathType Container) {
            Copy-Item $source $dest -Recurse -ErrorAction SilentlyContinue
        } else {
            Copy-Item $source $dest -ErrorAction SilentlyContinue
        }
    }
}

# Kopiere vcpkg_packages für Docker COPY
Write-Step "Copying pre-built packages to build context..."
$packagesInContext = Join-Path $buildContextDir "vcpkg_packages"
New-Item -Path $packagesInContext -ItemType Directory -Force | Out-Null
New-Item -Path (Join-Path $packagesInContext "x64-linux\$Configuration") -ItemType Directory -Force | Out-Null
Copy-Item $linuxPackagesDir\* (Join-Path $packagesInContext "x64-linux\$Configuration") -Recurse -Force

$contextSize = (Get-ChildItem $buildContextDir -Recurse -File | Measure-Object -Property Length -Sum).Sum / 1MB
Write-Host "  ✓ Build context ready: $([math]::Round($contextSize, 0)) MB" -ForegroundColor Green

# =============================================================================
# Docker Build
# =============================================================================

Write-Header "Starting Docker Build"

Write-Host @"
Build Configuration:
  Edition:       $Edition
  Tag:           $Tag
  Configuration: $Configuration
  LLM Enabled:   $EnableLLM
  GPU Enabled:   $EnableGPU
  Build Cache:   $(if ($NoBuildCache) { 'DISABLED' } else { 'ENABLED' })
  Log File:      $LogFile

"@ -ForegroundColor Cyan

$env:DOCKER_BUILDKIT = "1"

$buildArgs = @(
    "build",
    "-t", $Tag,
    "-f", "Dockerfile.prebuilt",
    "--progress=plain",
    "--build-arg", "THEMIS_EDITION=$Edition",
    "--build-arg", "ENABLE_LLM=$EnableLLM",
    "--build-arg", "ENABLE_GPU=$EnableGPU",
    "--build-arg", "BUILD_TYPE=$(if ($Configuration -eq 'debug') { 'Debug' } else { 'Release' })"
)

if ($NoBuildCache) {
    $buildArgs += "--no-cache"
}

$buildArgs += $buildContextDir

Write-Step "Executing: docker $($buildArgs -join ' ')"

# Build mit Logging
docker @buildArgs 2>&1 | Tee-Object -FilePath $LogFile

if ($LASTEXITCODE -ne 0) {
    Write-Error-Message "Docker build failed!"
    Write-Host "`nCheck log file: $LogFile" -ForegroundColor Yellow
    Write-Host "Last 30 lines:" -ForegroundColor Yellow
    Get-Content $LogFile -Tail 30
    exit 1
}

# =============================================================================
# Success Summary
# =============================================================================

Write-Header "Build Complete"

$imageInfo = docker images $Tag --format "{{.Repository}}:{{.Tag}} | {{.Size}}" | Select-Object -First 1

Write-Host "✓ Docker image built successfully!" -ForegroundColor Green
Write-Host "  Image: $imageInfo" -ForegroundColor Green
Write-Host "  Log:   $LogFile" -ForegroundColor Cyan

Write-Host "`nNext Steps:" -ForegroundColor Yellow
Write-Host "  Test: docker run --rm $Tag themis_server --version"
Write-Host "  Run:  docker run -d -p 8080:8080 -p 9090:9090 --name themisdb $Tag"
Write-Host "  Logs: docker logs -f themisdb"

Write-Host "`nPerformance Note:" -ForegroundColor Cyan
Write-Host "  This build used PRE-BUILT packages → significantly faster than vcpkg install"
Write-Host "  Future rebuilds reuse the same packages for maximum speed"
