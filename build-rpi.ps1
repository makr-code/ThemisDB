# Build Raspberry Pi ARM64 Docker image and extract binary
# Requires Docker with buildx support

param(
    [string]$Version = "1.0.0",
    [string]$Platform = "linux/arm64",
    [switch]$Push = $false
)

$ErrorActionPreference = "Stop"

Write-Host "=== ThemisDB Raspberry Pi ARM64 Build ===" -ForegroundColor Blue
Write-Host ""
Write-Host "Version: $Version"
Write-Host "Platform: $Platform"
Write-Host ""

# Ensure buildx is available
Write-Host "Checking Docker buildx..." -ForegroundColor Cyan
docker buildx version
if ($LASTEXITCODE -ne 0) {
    Write-Error "Docker buildx not available. Please install/enable it."
    exit 1
}

# Create builder if it doesn't exist
$builderName = "themis-multiarch"
$existingBuilder = docker buildx ls | Select-String $builderName
if (-not $existingBuilder) {
    Write-Host "Creating multiarch builder..." -ForegroundColor Cyan
    docker buildx create --name $builderName --use --platform linux/amd64,linux/arm64,linux/arm/v7
} else {
    Write-Host "Using existing builder: $builderName" -ForegroundColor Cyan
    docker buildx use $builderName
}

# Build ARM64 image
Write-Host ""
Write-Host "Building ARM64 Docker image..." -ForegroundColor Green
Write-Host "(This may take 30-60 minutes on first build)" -ForegroundColor Yellow
Write-Host ""

$imageName = "themisdb/themisdb:$Version-rpi"
$imageLatest = "themisdb/themisdb:rpi"

$buildArgs = @(
    "buildx", "build",
    "--platform", $Platform,
    "--build-arg", "TARGETARCH=arm64",
    "--build-arg", "VCPKG_TRIPLET=arm64-linux",
    "--progress", "plain",
    "-t", $imageName,
    "-t", $imageLatest,
    "-f", "Dockerfile"
)

if ($Push) {
    $buildArgs += "--push"
} else {
    $buildArgs += "--load"
}

$buildArgs += "."

& docker @buildArgs

if ($LASTEXITCODE -ne 0) {
    Write-Error "Docker build failed!"
    exit 1
}

Write-Host ""
Write-Host "ARM64 image built successfully: $imageName" -ForegroundColor Green

# Extract binary if not pushing (local build)
if (-not $Push) {
    Write-Host ""
    Write-Host "Extracting binary from ARM64 image..." -ForegroundColor Cyan
    
    # Create temp container
    $containerId = docker create --platform $Platform $imageName
    
    # Extract binary
    $outputDir = "release\themisdb-$Version-rpi-arm64"
    New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
    
    docker cp "${containerId}:/usr/local/bin/themis_server" "$outputDir\themis_server_rpi_arm64"
    docker rm $containerId | Out-Null
    
    # Copy additional files
    Copy-Item "LICENSE" -Destination $outputDir
    Copy-Item "README.md" -Destination $outputDir
    Copy-Item "config" -Destination $outputDir -Recurse -Force
    
    # Create INSTALL.txt
    @"
ThemisDB v$Version - Raspberry Pi ARM64

Installation:
1. Ensure Raspberry Pi OS Bullseye/Bookworm (64-bit)
2. Install dependencies:
   sudo apt-get update
   sudo apt-get install libssl3 libcurl4 libyaml-cpp0.7

3. Make binary executable:
   chmod +x themis_server_rpi_arm64

4. Run:
   ./themis_server_rpi_arm64

Server listens on http://localhost:18765

System Requirements:
- Raspberry Pi 4/5 (recommended: 4GB+ RAM)
- Raspberry Pi OS 64-bit (Bullseye or Bookworm)
- 2GB+ free disk space
"@ | Out-File -FilePath "$outputDir\INSTALL.txt" -Encoding UTF8
    
    # Create SHA256SUMS.txt
    $binaryHash = (Get-FileHash -Path "$outputDir\themis_server_rpi_arm64" -Algorithm SHA256).Hash
    @"
$binaryHash  themis_server_rpi_arm64
"@ | Out-File -FilePath "$outputDir\SHA256SUMS.txt" -Encoding ASCII
    
    # Create ZIP package
    Write-Host ""
    Write-Host "Creating release package..." -ForegroundColor Cyan
    $zipPath = "release\themisdb-$Version-rpi-arm64.zip"
    Compress-Archive -Path "$outputDir\*" -DestinationPath $zipPath -Force
    
    # Generate ZIP checksum
    $zipHash = (Get-FileHash -Path $zipPath -Algorithm SHA256).Hash
    $zipHash | Out-File -FilePath "$zipPath.sha256" -Encoding ASCII
    
    Write-Host ""
    Write-Host "=== Build Complete ===" -ForegroundColor Green
    Write-Host ""
    Write-Host "Package: $zipPath" -ForegroundColor Cyan
    Write-Host "SHA256: $zipHash" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Docker image: $imageName" -ForegroundColor Cyan
    Write-Host ""
    
} else {
    Write-Host ""
    Write-Host "Image pushed to registry: $imageName" -ForegroundColor Green
    Write-Host ""
}

Write-Host "To run on Raspberry Pi:" -ForegroundColor Yellow
Write-Host "  docker run -d -p 18765:18765 -v /path/to/data:/data $imageName" -ForegroundColor White
Write-Host ""
