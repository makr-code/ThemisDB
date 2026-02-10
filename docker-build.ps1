# Docker Build Script for ThemisDB
# This script temporarily moves the vcpkg directory to avoid including it in the build context
# The Dockerfile clones a fresh vcpkg anyway, so the local copy isn't needed

param(
    [string]$Target = "runtime",
    [string]$Tag = "themisdb:latest",
    [string]$Edition = "COMMUNITY"
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "ThemisDB Docker Build" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Target:  $Target"
Write-Host "Tag:     $Tag"
Write-Host "Edition: $Edition"
Write-Host ""

# Check if vcpkg directory exists
$vcpkgExists = Test-Path "vcpkg" -PathType Container
$vcpkgInstalledExists = Test-Path "vcpkg_installed" -PathType Container
$renamedVcpkg = $false
$renamedVcpkgInstalled = $false

try {
    # Temporarily rename vcpkg directories to exclude them from build context
    if ($vcpkgExists) {
        Write-Host "Temporarily moving vcpkg directory..." -ForegroundColor Yellow
        Rename-Item "vcpkg" "vcpkg.tmp" -ErrorAction Stop
        $renamedVcpkg = $true
    }
    
    if ($vcpkgInstalledExists) {
        Write-Host "Temporarily moving vcpkg_installed directory..." -ForegroundColor Yellow
        Rename-Item "vcpkg_installed" "vcpkg_installed.tmp" -ErrorAction Stop
        $renamedVcpkgInstalled = $true
    }
    
    # Run Docker build
    Write-Host "`nStarting Docker build..." -ForegroundColor Green
    $buildArgs = @(
        "build",
        ".",
        "--target=$Target",
        "--tag=$Tag",
        "--build-arg", "THEMIS_EDITION=$Edition"
    )
    
    & docker @buildArgs
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`nBuild successful!" -ForegroundColor Green
        Write-Host "Image tagged as: $Tag"
    } else {
        Write-Host "`nBuild failed with exit code: $LASTEXITCODE" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}
finally {
    # Restore vcpkg directories
    if ($renamedVcpkg -and (Test-Path "vcpkg.tmp")) {
        Write-Host "`nRestoring vcpkg directory..." -ForegroundColor Yellow
        Rename-Item "vcpkg.tmp" "vcpkg" -ErrorAction SilentlyContinue
    }
    
    if ($renamedVcpkgInstalled -and (Test-Path "vcpkg_installed.tmp")) {
        Write-Host "Restoring vcpkg_installed directory..." -ForegroundColor Yellow
        Rename-Item "vcpkg_installed.tmp" "vcpkg_installed" -ErrorAction SilentlyContinue
    }
}

Write-Host "`nDone!" -ForegroundColor Cyan
