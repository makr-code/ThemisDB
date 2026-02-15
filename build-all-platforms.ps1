<#
.SYNOPSIS
    Orchestrate complete multi-platform build workflow
    
.DESCRIPTION
    Vereinfachter Workflow für:
    1. Build vcpkg packages (Windows + Linux, Debug + Release)
    2. Build Docker image mit mounted packages
    3. Test Docker image

.PARAMETER Stage
    Build-Stage: packages, docker, test, oder all (default: all)

.PARAMETER Edition
    ThemisDB Edition (default: COMMUNITY)

.PARAMETER Configuration
    Build-Konfiguration: debug, release, oder all (default: release)

.PARAMETER Quick
    Nur Release builds, skip tests

.EXAMPLE
    .\build-all-platforms.ps1
    .\build-all-platforms.ps1 -Stage packages -Configuration all
    .\build-all-platforms.ps1 -Stage docker -Edition ENTERPRISE
    .\build-all-platforms.ps1 -Quick
#>

param(
    [Parameter()]
    [ValidateSet('packages', 'docker', 'test', 'all')]
    [string]$Stage = 'all',
    
    [Parameter()]
    [ValidateSet('COMMUNITY', 'MINIMAL', 'ENTERPRISE', 'HYPERSCALER')]
    [string]$Edition = 'COMMUNITY',
    
    [Parameter()]
    [ValidateSet('debug', 'release', 'all')]
    [string]$Configuration = 'release',
    
    [Parameter()]
    [switch]$Quick
)

$ErrorActionPreference = 'Stop'

function Write-Header {
    param([string]$Text)
    Write-Host "`n$('='*80)" -ForegroundColor Cyan
    Write-Host "  $Text" -ForegroundColor Cyan
    Write-Host "$('='*80)`n" -ForegroundColor Cyan
}

function Write-Step {
    param([string]$Text)
    Write-Host "`n>>> $Text" -ForegroundColor Green
}

$startTime = Get-Date

# =============================================================================
# Stage 1: Build Packages
# =============================================================================

if ($Stage -eq 'packages' -or $Stage -eq 'all') {
    Write-Header "Stage 1: Build vcpkg Packages"
    
    $packageArgs = @{
        Platform = 'all'
        Configuration = if ($Quick) { 'release' } else { $Configuration }
        Edition = $Edition
    }
    
    Write-Step "Building packages for all platforms..."
    & "$PSScriptRoot\build-vcpkg-packages.ps1" @packageArgs
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Package build failed!" -ForegroundColor Red
        exit 1
    }
}

# =============================================================================
# Stage 2: Build Docker Image
# =============================================================================

if ($Stage -eq 'docker' -or $Stage -eq 'all') {
    Write-Header "Stage 2: Build Docker Image"
    
    # Build für jede gewünschte Konfiguration
    $configs = if ($Quick -or $Configuration -eq 'release') { @('release') } 
               elseif ($Configuration -eq 'all') { @('debug', 'release') }
               else { @($Configuration) }
    
    foreach ($config in $configs) {
        Write-Step "Building Docker image ($config)..."
        
        $tag = "themisdb:$($Edition.ToLower())-$config"
        
        & "$PSScriptRoot\docker-build-with-prebuilt-packages.ps1" `
            -Edition $Edition `
            -Tag $tag `
            -Configuration $config
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "ERROR: Docker build failed for $config!" -ForegroundColor Red
            exit 1
        }
    }
}

# =============================================================================
# Stage 3: Test Docker Image
# =============================================================================

if (($Stage -eq 'test' -or $Stage -eq 'all') -and !$Quick) {
    Write-Header "Stage 3: Test Docker Image"
    
    $configs = if ($Configuration -eq 'all') { @('debug', 'release') } else { @($Configuration) }
    
    foreach ($config in $configs) {
        $tag = "themisdb:$($Edition.ToLower())-$config"
        
        Write-Step "Testing image: $tag"
        
        # Test 1: Version check
        Write-Host "  Test: --version" -ForegroundColor Yellow
        docker run --rm $tag themis_server --version
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  ✗ FAILED: --version" -ForegroundColor Red
            continue
        }
        Write-Host "  ✓ PASSED: --version" -ForegroundColor Green
        
        # Test 2: Build info
        Write-Host "`n  Test: --build-info" -ForegroundColor Yellow
        docker run --rm $tag themis_server --build-info
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  ✗ FAILED: --build-info" -ForegroundColor Red
            continue
        }
        Write-Host "  ✓ PASSED: --build-info" -ForegroundColor Green
        
        # Test 3: Help
        Write-Host "`n  Test: --help" -ForegroundColor Yellow
        docker run --rm $tag themis_server --help | Select-Object -First 10
        if ($LASTEXITCODE -ne 0) {
            Write-Host "  ✗ FAILED: --help" -ForegroundColor Red
            continue
        }
        Write-Host "  ✓ PASSED: --help" -ForegroundColor Green
        
        Write-Host "`n✓ All tests passed for $tag" -ForegroundColor Green
    }
}

# =============================================================================
# Summary
# =============================================================================

Write-Header "Build Summary"

$elapsed = (Get-Date) - $startTime
Write-Host "Total time: $($elapsed.ToString('hh\:mm\:ss'))" -ForegroundColor Green

Write-Host "`nBuilt Images:" -ForegroundColor Cyan
docker images themisdb --format "table {{.Repository}}\t{{.Tag}}\t{{.Size}}\t{{.CreatedAt}}" | Out-String | Write-Host

Write-Host "`nPackage Store:" -ForegroundColor Cyan
$packageStore = Join-Path $PSScriptRoot "vcpkg_packages"
if (Test-Path $packageStore) {
    $totalSize = (Get-ChildItem $packageStore -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum / 1GB
    Write-Host "  Location: $packageStore" -ForegroundColor White
    Write-Host "  Size:     $([math]::Round($totalSize, 2)) GB" -ForegroundColor White
    
    Get-ChildItem $packageStore -Recurse -Directory -Depth 1 | ForEach-Object {
        $size = (Get-ChildItem $_.FullName -Recurse -File -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum / 1GB
        Write-Host "    - $($_.FullName -replace [regex]::Escape($packageStore)): $([math]::Round($size, 2)) GB" -ForegroundColor Gray
    }
}

Write-Host "`n✓ Multi-platform build complete!" -ForegroundColor Green
Write-Host "`nUsage Examples:" -ForegroundColor Yellow
Write-Host "  # Run release image"
Write-Host "  docker run -d -p 8080:8080 -p 9090:9090 --name themisdb themisdb:$($Edition.ToLower())-release"
Write-Host ""
Write-Host "  # Run debug image with verbose logging"
Write-Host "  docker run -it -p 8080:8080 themisdb:$($Edition.ToLower())-debug themis_server --log-level=debug"
Write-Host ""
Write-Host "  # Rebuild packages only"
Write-Host "  .\build-all-platforms.ps1 -Stage packages"
Write-Host ""
Write-Host "  # Rebuild Docker only (reuse existing packages)"
Write-Host "  .\build-all-platforms.ps1 -Stage docker"
