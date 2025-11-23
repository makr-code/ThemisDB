# Build QNAP-compatible Docker Image using pre-built binary
# Uses Ubuntu 20.04 runtime for GLIBC 2.31 compatibility

param(
    [string]$Tag = "themisdb:qnap"
)

$ErrorActionPreference = "Stop"

Write-Host "=== ThemisDB QNAP Docker Build (Simple) ===" -ForegroundColor Cyan
Write-Host "Strategy: Copy WSL binary to Ubuntu 20.04 runtime" -ForegroundColor Yellow
Write-Host ""

# Note: This requires WSL binary to be compatible with Ubuntu 20.04
# If WSL uses newer Ubuntu (22.04+), binary won't work on QNAP
$wslVersion = wsl bash -lc "lsb_release -rs"
Write-Host "WSL Ubuntu Version: $wslVersion" -ForegroundColor Gray

if ($wslVersion -notmatch "^20\.") {
    Write-Host ""
    Write-Host "WARNING: WSL is running Ubuntu $wslVersion" -ForegroundColor Red
    Write-Host "Binary built on Ubuntu $wslVersion requires GLIBC 2.35+" -ForegroundColor Red
    Write-Host "QNAP needs Ubuntu 20.04 (GLIBC 2.31)" -ForegroundColor Red
    Write-Host ""
    Write-Host "Solution: Use full Dockerfile.qnap build (slow but correct)" -ForegroundColor Yellow
    Write-Host "  .\build-docker-qnap.ps1" -ForegroundColor Gray
    exit 1
}

# Step 1: Check WSL binary
Write-Host "Step 1: Checking WSL build..." -ForegroundColor Yellow
$wslBinaryExists = wsl bash -lc "test -f ~/themis-build-release/themis_server && echo 'exists' || echo 'missing'"

if ($wslBinaryExists -match "missing") {
    Write-Host "Binary not found. Building in WSL..." -ForegroundColor Yellow
    wsl bash -lc "cd ~/themis-build-release && cmake --build . --target themis_server -j`$(nproc)"
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "WSL build failed!" -ForegroundColor Red
        exit 1
    }
}

Write-Host "Binary ready at ~/themis-build-release/themis_server" -ForegroundColor Green

# Step 2: Copy binary to Windows
Write-Host ""
Write-Host "Step 2: Copying binary to Windows..." -ForegroundColor Yellow
$winBuildDir = "build"
if (-not (Test-Path $winBuildDir)) {
    New-Item -ItemType Directory -Path $winBuildDir | Out-Null
}

wsl bash -lc "cp ~/themis-build-release/themis_server /mnt/c/VCC/themis/build/"

if (-not (Test-Path "build\themis_server")) {
    Write-Host "Failed to copy binary!" -ForegroundColor Red
    exit 1
}

Write-Host "Binary copied successfully" -ForegroundColor Green

# Step 3: Create Dockerfile for Ubuntu 20.04
Write-Host ""
Write-Host "Step 3: Creating temporary Dockerfile..." -ForegroundColor Yellow

$dockerfileContent = @"
FROM ubuntu:20.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

COPY build/themis_server /usr/local/bin/themis_server

RUN mkdir -p /etc/themis /usr/local/share/themis /data /var/log/themis && \
    chmod +x /usr/local/bin/themis_server

ENV THEMIS_CONFIG_PATH=/etc/themis/config.json
ENV THEMIS_PORT=18765

VOLUME ["/data"]
EXPOSE 8080 18765

ENTRYPOINT ["/usr/local/bin/themis_server"]
CMD ["--config", "/etc/themis/config.json"]
"@

$dockerfileContent | Out-File -FilePath "Dockerfile.qnap.simple.tmp" -Encoding UTF8 -NoNewline

# Step 4: Build Docker image
Write-Host ""
Write-Host "Step 4: Building Docker image with Ubuntu 20.04..." -ForegroundColor Yellow
docker build -f Dockerfile.qnap.simple.tmp -t $Tag .

if ($LASTEXITCODE -eq 0) {
    Remove-Item "Dockerfile.qnap.simple.tmp" -ErrorAction SilentlyContinue
    
    Write-Host ""
    Write-Host "=== Build erfolgreich ===" -ForegroundColor Green
    Write-Host "Image: $Tag" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "GLIBC-Kompatibilität prüfen:" -ForegroundColor Yellow
    Write-Host "  docker run --rm $Tag ldd /usr/local/bin/themis_server | Select-String GLIBC" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Testen:" -ForegroundColor Yellow
    Write-Host "  docker run --rm $Tag --version" -ForegroundColor Gray
} else {
    Remove-Item "Dockerfile.qnap.simple.tmp" -ErrorAction SilentlyContinue
    Write-Host ""
    Write-Host "=== Build fehlgeschlagen ===" -ForegroundColor Red
    exit 1
}
