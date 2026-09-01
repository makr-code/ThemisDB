#!/usr/bin/env pwsh
<#
.SYNOPSIS
    ThemisDB Docker Image Build & Push Script
.DESCRIPTION
    Builds Docker images for ThemisDB and optionally pushes to Docker Hub.
    Supports both pre-built binaries and full Docker builds.
.EXAMPLE
    .\build-docker.ps1 -Tag "1.3.4" -Push
    .\build-docker.ps1 -Platforms "linux/amd64,linux/arm64" -Tag "1.3.4"
#>

param(
    [string]$Platforms = "linux/amd64",
    [string]$Tag = (Get-Content -Path (Join-Path (Split-Path -Parent $PSScriptRoot) "VERSION") -ErrorAction SilentlyContinue | Select-Object -First 1).Trim(),
    [string]$Dockerfile = "Dockerfile.unified",
    [switch]$Push,
    [switch]$NoCache,
    [switch]$BuildBinary
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$releaseDir = Join-Path $repoRoot "release"
$multiArchOciPath = Join-Path $releaseDir "themisdb-$Tag-multiarch.oci.tar"

# Validation
if (-not $Tag) {
    Write-Host "ERROR: Version tag required. Provide -Tag or ensure VERSION file exists." -ForegroundColor Red
    exit 1
}

Write-Host "╔════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║     ThemisDB Docker Build Script      ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""
Write-Host "Configuration:" -ForegroundColor Green
Write-Host "  Tag:       $Tag"
Write-Host "  Platforms: $Platforms"
Write-Host "  Dockerfile: $Dockerfile"
Write-Host "  Push:      $(if ($Push) { 'YES' } else { 'NO' })"
Write-Host "  Build Binary: $(if ($BuildBinary) { 'YES' } else { 'NO' })"
Write-Host ""

# Step 1: Build binary if requested
if ($BuildBinary) {
    Write-Host "════════ Building themis_server binary ════════" -ForegroundColor Yellow

    Push-Location $repoRoot

    Write-Host "Configuring CMake..." -ForegroundColor Cyan
    & cmake --preset windows-release `
        -DTHEMIS_BUILD_TESTS=OFF `
        -DTHEMIS_BUILD_BENCHMARKS=OFF
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed" }

    Write-Host "Building themis_server..." -ForegroundColor Cyan
    & cmake --build --preset windows-release --target themis_server
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }

    Pop-Location

    # Copy binary to release directory
    if (-not (Test-Path $releaseDir)) { New-Item -ItemType Directory -Path $releaseDir | Out-Null }
    $binaryPath = Join-Path $repoRoot "build-msvc-windows-release/themis_server.exe"
    if (Test-Path $binaryPath) {
        Write-Host "Copying binary to $releaseDir..." -ForegroundColor Cyan
        Copy-Item -Path $binaryPath -Destination (Join-Path $releaseDir "themis_server.exe") -Force
    }
}

# Step 2: Check Docker installation
Write-Host ""
Write-Host "════════ Checking Docker installation ════════" -ForegroundColor Yellow
$dockerVersion = docker --version 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Docker not found or not running" -ForegroundColor Red
    exit 1
}
Write-Host $dockerVersion -ForegroundColor Green

# Step 3: Build Docker image
Write-Host ""
Write-Host "════════ Building Docker image ════════" -ForegroundColor Yellow

$imageTag = "themisdb/themis:$Tag"
$imageTagLatest = "themisdb/themis:latest"

$proxyBuildArgs = @()
foreach ($name in @("HTTP_PROXY", "HTTPS_PROXY", "ALL_PROXY", "NO_PROXY", "VCPKG_ENABLE_ONLINE", "INCLUDE_TINYLLAMA", "TINYLLAMA_FORCE_DOWNLOAD", "TINYLLAMA_HF_URL")) {
    $value = [Environment]::GetEnvironmentVariable($name)
    if (-not [string]::IsNullOrWhiteSpace($value)) {
        $proxyBuildArgs += "--build-arg"
        $proxyBuildArgs += "$name=$value"
    }
}

# For multi-arch builds, use buildx
if ($Platforms -match ",") {
    Write-Host "Building multi-architecture image..." -ForegroundColor Cyan
    $buildxList = & docker "buildx" "ls"
    if ($LASTEXITCODE -ne 0) {
        $buildxList = $null
    }
    if (-not $buildxList) {
        Write-Host "Setting up buildx builder..." -ForegroundColor Cyan
        & docker "buildx" "create" "--name" "themis-builder" "--use"
    }
    
    $buildxArgs = @(
        "buildx", "build",
        "--builder", "themis-builder",
        "-f", (Join-Path $repoRoot "docker/$Dockerfile"),
        "-t", $imageTag,
        "-t", $imageTagLatest,
        "--platform", $Platforms,
        "--build-arg", "VERSION=$Tag"
    )
    if ($proxyBuildArgs.Count -gt 0) { $buildxArgs += $proxyBuildArgs }
    if ($NoCache) { $buildxArgs += "--no-cache" }
    if ($Push) {
        $buildxArgs += "--push"
        Write-Host "  [Push enabled - image will be pushed after build]" -ForegroundColor Green
    } else {
        if (-not (Test-Path $releaseDir)) { New-Item -ItemType Directory -Path $releaseDir | Out-Null }
        if (Test-Path $multiArchOciPath) { Remove-Item -Path $multiArchOciPath -Force }
        $buildxArgs += "--output=type=oci,dest=$multiArchOciPath"
        Write-Host "  [Push disabled - exporting multi-arch OCI archive to $multiArchOciPath]" -ForegroundColor Yellow
    }
    
    & docker @buildxArgs $repoRoot
    if ($LASTEXITCODE -ne 0) { throw "Multi-arch Docker build failed" }
} else {
    # Single-arch build with standard docker build
    $dockerArgs = @(
        "build",
        "-f", (Join-Path $repoRoot "docker/$Dockerfile"),
        "-t", $imageTag,
        "-t", $imageTagLatest,
        "--build-arg", "VERSION=$Tag"
    )
    if ($proxyBuildArgs.Count -gt 0) { $dockerArgs += $proxyBuildArgs }
    if ($NoCache) { $dockerArgs += "--no-cache" }
    
    & docker @dockerArgs $repoRoot
    if ($LASTEXITCODE -ne 0) { throw "Docker build failed" }
    
    Write-Host "Image built successfully: $imageTag" -ForegroundColor Green
    
    # Step 5: Push to Docker Hub (if requested)
    if ($Push) {
        Write-Host ""
        Write-Host "════════ Pushing to Docker Hub ════════" -ForegroundColor Yellow
        
        # Check Docker login
        $loginStatus = docker info 2>$null | Select-String "Username"
        if (-not $loginStatus) {
            Write-Host "Docker not logged in. Running: docker login" -ForegroundColor Cyan
            & docker login
            if ($LASTEXITCODE -ne 0) { throw "Docker login failed" }
        }
        
        # Push image and tags
        Write-Host "Pushing $imageTag..." -ForegroundColor Cyan
        & docker push $imageTag
        if ($LASTEXITCODE -ne 0) { throw "Docker push failed for $imageTag" }
        
        Write-Host "Pushing $imageTagLatest..." -ForegroundColor Cyan
        & docker push $imageTagLatest
        if ($LASTEXITCODE -ne 0) { throw "Docker push failed for $imageTagLatest" }
        
        Write-Host "✅ Images pushed successfully!" -ForegroundColor Green
    }
}

# Step 6: Summary
Write-Host ""
Write-Host "╔════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║    Docker Build Completed Successfully ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""
Write-Host "Image Tags:" -ForegroundColor Cyan
Write-Host "  • $imageTag"
Write-Host "  • $imageTagLatest"
Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Cyan
if ($Push) {
    Write-Host "  ✅ Image has been pushed to Docker Hub"
    Write-Host "  → Pull with: docker pull $imageTag"
} else {
    Write-Host "  → Run locally: docker run -d -p 18765:18765 -v themisdb_data:/data $imageTag"
    Write-Host "  → Push to registry: docker push $imageTag"
}
Write-Host ""
