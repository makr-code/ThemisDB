# Build ThemisDB Binary on WSL and create Docker Image
# This bypasses vcpkg download issues by using local build

param(
    [string]$BuildDir = "$env:HOME/themis-build-release",
    [string]$Tag = "themisdb:latest"
)

$ErrorActionPreference = "Stop"

Write-Host "=== ThemisDB Simple Docker Build ===" -ForegroundColor Cyan
Write-Host ""

# Step 1: Build on WSL if not already built
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

# Step 2: Copy binary to Windows build directory
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

# Step 3: Build Docker image
Write-Host ""
Write-Host "Step 3: Building Docker image..." -ForegroundColor Yellow
docker build -f Dockerfile.simple -t $Tag .

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "=== Build erfolgreich ===" -ForegroundColor Green
    Write-Host "Image: $Tag" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Testen:" -ForegroundColor Yellow
    Write-Host "  docker run --rm $Tag --version" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Starten:" -ForegroundColor Yellow
    Write-Host "  docker run -p 18765:18765 -v ${PWD}/data:/data $Tag" -ForegroundColor Gray
} else {
    Write-Host ""
    Write-Host "=== Build fehlgeschlagen ===" -ForegroundColor Red
    exit 1
}
