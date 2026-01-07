# Build ThemisDB Community Edition v1.3.5
# ========================================
# Builds the free, open-source Community Edition for multiple platforms.
# Edition features: 24GB GPU VRAM, single-node, LLM core features (Embedding, Similarity, Inference), no enterprise plugins.

param(
    [ValidateSet("windows", "docker", "all")]
    [string]$Platform = "all",
    
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    
    [switch]$SkipTests,
    [switch]$SkipDocker
)
$ErrorActionPreference = "Stop"
$BuildDir = "C:\VCC\themis\build-msvc"
$Version = "1.3.5"
$EditionName = "community"
$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building ThemisDB v$Version - COMMUNITY Edition" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Platform: $Platform"
Write-Host "Configuration: $Configuration"
Write-Host "Edition: COMMUNITY (GPU: 24GB, Nodes: 1, Plugins: No, LLM: Yes)"
Write-Host ""

# Function: Configure CMake with edition settings
function Configure-CMake-Community {
    param(
        [string]$OutputDir = $BuildDir
    )
    
    Write-Host "Configuring CMake for Community Edition..." -ForegroundColor Yellow
    if (!(Test-Path $OutputDir)) {
        New-Item -ItemType Directory -Path $OutputDir | Out-Null
    }
    Push-Location $OutputDir
    try {
        $env:VCPKG_ROOT = "C:\VCC\themis\vcpkg"
        # Configure with COMMUNITY edition
        cmake -S C:\VCC\themis -B . `
            -G "Visual Studio 17 2022" `
            -A x64 `
            -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
            -DVCPKG_TARGET_TRIPLET=x64-windows `
            -DTHEMIS_EDITION=COMMUNITY `
            -DCMAKE_BUILD_TYPE=$Configuration `
            -DTHEMIS_BUILD_TESTS=ON `
            -DTHEMIS_BUILD_BENCHMARKS=ON `
            -DTHEMIS_ENABLE_GPU=ON `
            -DTHEMIS_ENABLE_TRACING=ON `
            -DBUILD_SHARED_LIBS=ON
        
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }
    }
    finally {
        Pop-Location
    }
}

# Function: Build Community Edition
function Build-Community {
    param(
        [string]$OutputDir = $BuildDir,
        [string]$Config = "Release"
    )
    
    Write-Host "Building Community Edition..." -ForegroundColor Yellow
    Push-Location $OutputDir
    try {
        cmake --build . --config $Config --parallel 8
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }
    }
    finally {
        Pop-Location
    }
}

# Function: Generate SHA256 checksums
function Generate-Checksums {
    param(
        [string[]]$Files
    )
    
    Write-Host "Generating SHA256 checksums..." -ForegroundColor Yellow
    
    $checksum_file = "C:\VCC\themis\SHA256SUMS"
    "" | Out-File $checksum_file
    
    foreach ($file in $Files) {
        if (Test-Path $file) {
            $hash = (Get-FileHash -Path $file -Algorithm SHA256).Hash
            "$hash  $(Split-Path -Leaf $file)" | Add-Content $checksum_file
            Write-Host "  $hash" -ForegroundColor Gray
        }
    }
}

# Function: Create release artifacts
function Create-Release-Artifacts {
    param(
        [string]$OutputDir = $BuildDir,
        [string]$Config = "Release"
    )
    
    Write-Host "Creating release artifacts..." -ForegroundColor Yellow
    $artifact_dir = "C:\VCC\themis\release\v$Version\$EditionName-windows-x64"
    New-Item -ItemType Directory -Path $artifact_dir -Force | Out-Null
    
    # Copy executable and dependencies
    $exe_path = Join-Path $OutputDir $Config "themis_server.exe"
    if (Test-Path $exe_path) {
        Copy-Item $exe_path $artifact_dir\
        Write-Host "  Copied themis_server.exe" -ForegroundColor Green
    } else {
        Write-Host "  WARNING: themis_server.exe not found" -ForegroundColor Red
    }
    
    # Copy libraries (DLLs)
    Get-ChildItem (Join-Path $OutputDir $Config) -Filter "*.dll" | ForEach-Object {
        Copy-Item $_.FullName $artifact_dir\
    }
    
    # Create release info file
    $info = @"
ThemisDB Community Edition v$Version
======================================
Build Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Edition: COMMUNITY
GPU VRAM Limit: 24 GB
Sharding: Single-node only
Enterprise Plugins: Disabled
LLM Features: Enabled (llama.cpp - Embedding, Similarity, Inference)

Release Notes: See RELEASE_NOTES_v$Version.md
"@
    $info | Out-File (Join-Path $artifact_dir "EDITION_INFO.txt")
    
    Write-Host "  Release artifacts created at: $artifact_dir" -ForegroundColor Green
    return $artifact_dir
}

# Main build flow for Windows
if ($Platform -eq "windows" -or $Platform -eq "all") {
    Write-Host "`nSTEP 1: Configure CMake" -ForegroundColor Cyan
    Configure-CMake-Community
    
    Write-Host "`nSTEP 2: Build Release" -ForegroundColor Cyan
    Build-Community -Config $Configuration
    
    Write-Host "`nSTEP 3: Create Artifacts" -ForegroundColor Cyan
    $artifact_path = Create-Release-Artifacts -Config $Configuration
    
    Write-Host "`nSTEP 4: Generate Checksums" -ForegroundColor Cyan
    Generate-Checksums @((Join-Path $artifact_path "themis_server.exe"))
    
    if (-not $SkipTests) {
        Write-Host "`nSTEP 5: Run Tests" -ForegroundColor Cyan
        Push-Location $BuildDir
        try {
            ctest --build-config $Configuration --output-on-failure
        }
        finally {
            Pop-Location
        }
    }
}

# Docker build for Community Edition
if ($Platform -eq "docker" -or $Platform -eq "all") {
    if (-not $SkipDocker) {
        Write-Host "`nSTEP 6: Build Docker Image" -ForegroundColor Cyan
        Write-Host "Building Docker image for Community Edition..." -ForegroundColor Yellow
    $docker_tag = "themisdb:$Version-community-latest"
        
    # Build Docker image with COMMUNITY edition
        docker build -t $docker_tag `
            --build-arg THEMIS_EDITION=COMMUNITY `
            --build-arg THEMIS_VERSION=$Version `
            -f C:\VCC\themis\Dockerfile `
            C:\VCC\themis
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  Docker image built successfully: $docker_tag" -ForegroundColor Green
            
            Write-Host "`nSTEP 7: Push to Docker Hub (Public)" -ForegroundColor Cyan
            Write-Host "Pushing to docker.io/themisdb/themisdb:$Version-community..." -ForegroundColor Yellow
            
            docker tag $docker_tag "docker.io/themisdb/themisdb:$Version-community"
            docker push "docker.io/themisdb/themisdb:$Version-community"
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host "  Pushed to Docker Hub" -ForegroundColor Green
            } else {
                Write-Host "  WARNING: Docker Hub push failed" -ForegroundColor Red
            }
        } else {
            Write-Host "  ERROR: Docker build failed" -ForegroundColor Red
        }
    }
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Community Edition v$Version build complete!" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
# Build Script Template: Community Edition Release
# Location: scripts/build-community-release.ps1

param(
    [string]$OutputDir = ".\release\v1.3.0\community",
    [string]$BuildType = "Release",
    [int]$ParallelJobs = 8,
    [switch]$SkipDocker = $false
)

$ErrorActionPreference = "Stop"

Write-Host "=== ThemisDB Community Edition Release Build ===" -ForegroundColor Cyan

# 1. Setup
$env:VCPKG_ROOT = "$(Get-Location)\vcpkg"
if (-not (Test-Path $env:VCPKG_ROOT)) {
    throw "vcpkg not found. Run: .\scripts\setup-vcpkg-offline.ps1"
}

New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

# 2. Windows Build
Write-Host "`n[1/4] Building for Windows (x64)..." -ForegroundColor Yellow
$buildDir = "build-community-windows"
cmake -B $buildDir `
    -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
    -DTHEMIS_EDITION="COMMUNITY" `
    -DCMAKE_BUILD_TYPE=$BuildType `
    -DTHEMIS_BUILD_TESTS=OFF `
    -DTHEMIS_BUILD_BENCHMARKS=OFF

cmake --build $buildDir --config $BuildType --parallel $ParallelJobs

if (-not (Test-Path "$buildDir\$BuildType\themis_server.exe")) {
    throw "Windows build failed: themis_server.exe not found"
}

# Package Windows
$windowsPackage = "$OutputDir\windows\themisdb-1.3.0-windows-x64-community.zip"
New-Item -ItemType Directory -Path "$OutputDir\windows" -Force | Out-Null
Compress-Archive -Path @(
    "$buildDir\$BuildType\themis_server.exe",
    "$buildDir\$BuildType\*.dll"
) -DestinationPath $windowsPackage -Force
Write-Host "✓ Windows package: $windowsPackage" -ForegroundColor Green

# Generate SHA256
$hash = (Get-FileHash $windowsPackage -Algorithm SHA256).Hash
"$hash  themisdb-1.3.0-windows-x64-community.zip" | Out-File "$OutputDir\windows\SHA256SUMS" -Append

# 3. Docker Build
if (-not $SkipDocker) {
    Write-Host "`n[2/4] Building Docker image..." -ForegroundColor Yellow
    docker build -f Dockerfile.community `
        -t themisdb:1.3.0-community `
        -t themisdb:1.3.0-community-latest `
        --build-arg THEMIS_EDITION=COMMUNITY `
        .
    
    if ($LASTEXITCODE -ne 0) {
        throw "Docker build failed"
    }
    Write-Host "✓ Docker image: themisdb:1.3.0-community" -ForegroundColor Green
}

# 4. Generate Release Notes
Write-Host "`n[3/4] Generating release notes..." -ForegroundColor Yellow
@"
# ThemisDB Community Edition v1.3.0

**Release Date:** $(Get-Date -Format 'dd.MM.yyyy')

## Edition Information
- **Type:** Community (Open Source)
- **License:** MIT
- **Support:** Community (https://github.com/makr-code/ThemisDB/discussions)

## Features
- Core Database Engine (LSM-Tree)
- Vector Search (HNSW)
- Graph Queries
- Geospatial Queries
- Full-Text Search
- Time-Series Support
- JSON/Blob Storage
- Content Processing (Image, PDF, Audio, Video)

## Platforms
- Windows (x64)
- Linux (x64, ARM64)
- Docker (Multi-Arch)
- QNAP NAS

## SHA256 Checksums
$(Get-Content "$OutputDir\windows\SHA256SUMS" -Raw)

## Download
- https://github.com/makr-code/ThemisDB/releases
- https://hub.docker.com/r/themisdb/themisdb

## Installation
See: https://themisdb.io/docs/installation/community

## Changelog
See: CHANGELOG.md

---
Built with ❤️ by ThemisDB Team
"@ | Out-File "$OutputDir\RELEASE_NOTES_COMMUNITY.md"

Write-Host ✓ Release notes generated" -ForegroundColor Green

# 5. Summary
Write-Host "`n[4/4] Build Complete!" -ForegroundColor Green
Write-Host @"

Community Edition Release Summary:
  Package:     $(Resolve-Path $windowsPackage | Select-Object -ExpandProperty Path)
  Docker Tag:  themisdb:1.3.0-community
  Output Dir:  $(Resolve-Path $OutputDir | Select-Object -ExpandProperty Path)

Next Steps:
  1. Review: $OutputDir\RELEASE_NOTES_COMMUNITY.md
  2. Test:   docker run -it themisdb:1.3.0-community
  3. Push:   git tag v1.3.0-community && git push origin v1.3.0-community
  4. Upload: gh release create v1.3.0-community ...

"@
