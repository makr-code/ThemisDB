<#
.SYNOPSIS
    Build ThemisDB Docker image with vcpkg binary cache acceleration
    
.DESCRIPTION
    Optimized Docker build using:
    - vcpkg binary cache from %LOCALAPPDATA%\vcpkg\archives (26 GB, 1700+ zips)
    - Prebuilt vcpkg_installed libraries (11.5 GB) 
    - Optimized .dockerignore excluding symlink directories
    
.PARAMETER Edition
    ThemisDB edition: COMMUNITY, HYPERSCALER, ENTERPRISE
    
.PARAMETER Tag
    Docker image tag (default: themisdb:latest)
    
.PARAMETER UseBinaryCache
    Mount vcpkg binary cache for faster builds (default: true)
    
.PARAMETER UsePrebuilt
    Use local vcpkg_installed as prebuilt artifacts (default: true)
#>

[CmdletBinding()]
param(
    [Parameter()]
    [ValidateSet("COMMUNITY", "HYPERSCALER", "ENTERPRISE")]
    [string]$Edition = "COMMUNITY",
    
    [Parameter()]
    [string]$Tag = "themisdb:latest",
    
    [Parameter()]
    [bool]$UseBinaryCache = $true,
    
    [Parameter()]
    [bool]$UsePrebuilt = $true,
    
    [Parameter()]
    [switch]$NoBuildCache
)

$ErrorActionPreference = "Stop"

# Paths
$projectRoot = $PSScriptRoot
$vcpkgBinaryCache = Join-Path $env:LOCALAPPDATA "vcpkg\archives"
$vcpkgInstalled = Join-Path $projectRoot "vcpkg_installed"
$tempBuildContext = Join-Path $projectRoot ".docker-build-cached"
$logFile = Join-Path $projectRoot "docker-build-cached.log"

Write-Host "`n==========================================" -ForegroundColor Cyan
Write-Host "  ThemisDB Docker Build (Cached)" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "Edition:          $Edition" -ForegroundColor White
Write-Host "Tag:              $Tag" -ForegroundColor White
Write-Host "Binary Cache:     $UseBinaryCache" -ForegroundColor White
Write-Host "Use Prebuilt:     $UsePrebuilt" -ForegroundColor White
Write-Host "==========================================" -ForegroundColor Cyan

# Validate prerequisites
if ($UseBinaryCache -and -not (Test-Path $vcpkgBinaryCache)) {
    Write-Warning "vcpkg binary cache not found at: $vcpkgBinaryCache"
    Write-Warning "Disabling binary cache..."
    $UseBinaryCache = $false
}

# Disable prebuilt mode (too large to copy - uses binary cache instead)
if ($UsePrebuilt) {
    Write-Host "`nNote: Using binary cache instead of prebuilt vcpkg_installed" -ForegroundColor Yellow
    Write-Host "      (vcpkg_installed is 11.5 GB - binary cache is faster anyway)" -ForegroundColor Gray
    $UsePrebuilt = $false
}

# Show cache statistics
if ($UseBinaryCache) {
    $cacheCount = (Get-ChildItem $vcpkgBinaryCache -Recurse -Filter "*.zip" -ErrorAction SilentlyContinue).Count
    $cacheSize = [math]::Round((Get-ChildItem $vcpkgBinaryCache -Recurse -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum / 1GB, 2)
    Write-Host "`nBinary Cache: $cacheCount packages, $cacheSize GB" -ForegroundColor Green
    Write-Host "  → Will restore packages in seconds instead of compiling!" -ForegroundColor Cyan
}

Write-Host "`nCreating optimized build context..." -ForegroundColor Yellow

# Clean up previous temp context
if (Test-Path $tempBuildContext) {
    Remove-Item $tempBuildContext -Recurse -Force
}
New-Item -ItemType Directory -Path $tempBuildContext | Out-Null

# Copy optimized .dockerignore
$dockerignoreSource = if (Test-Path "$projectRoot\.dockerignore.optimized") {
    "$projectRoot\.dockerignore.optimized"
} else {
    "$projectRoot\.dockerignore"
}

Copy-Item $dockerignoreSource "$tempBuildContext\.dockerignore" -Force

# Essential files and directories for build
$itemsToCopy = @(
    "Dockerfile",
    "CMakeLists.txt",
    "VERSION",
    "RELEASE_TYPE",
    "vcpkg.json",
    "vcpkg-configuration.json",
    "cmake",
    "include",
    "src",
    "proto",
    "internal",
    "adapters",
    "aql",
    "docker",
    "llama.cpp",
    "ports",
    "tests"  # REQUIRED by CMakeLists.txt
)

# Add vcpkg directories (excluding symlinks - NO vcpkg_installed copy due to size!)
# Instead, we mount vcpkg binary cache for fast package restore
$itemsToCopy += "vcpkg"

Write-Host "Copying build context files..." -ForegroundColor Yellow
foreach ($item in $itemsToCopy) {
    $source = Join-Path $projectRoot $item
    $dest = Join-Path $tempBuildContext $item
    
    if (Test-Path $source) {
        if ((Get-Item $source).PSIsContainer) {
            # For vcpkg, only copy essential files
            if ($item -eq "vcpkg") {
                Write-Host "  > vcpkg (minimal: scripts, triplets, versions)" -ForegroundColor Gray
                
                # Create vcpkg directory structure
                New-Item -ItemType Directory -Path $dest -Force | Out-Null
                
                # Copy essential subdirectories
                @("scripts", "triplets", "versions") | ForEach-Object {
                    $vcpkgSub = Join-Path $source $_
                    if (Test-Path $vcpkgSub) {
                        Copy-Item $vcpkgSub (Join-Path $dest $_) -Recurse -Force
                    }
                }
                
                # Copy bootstrapper and config files
                Get-ChildItem $source -File | Where-Object { 
                    $_.Name -match "^(bootstrap|vcpkg).*\.(sh|bat|exe|json)$" 
                } | ForEach-Object {
                    Copy-Item $_.FullName $dest -Force
                }
                
                continue
            }
            
            Write-Host "  > $item" -ForegroundColor Gray
            Copy-Item $source $dest -Recurse -Force
        } else {
            Write-Host "  > $item" -ForegroundColor Gray
            Copy-Item $source $dest -Force
        }
    } else {
        Write-Host "  ! $item (not found, skipping)" -ForegroundColor DarkGray
    }
}

# Build docker command
$dockerArgs = @(
    "build"
    "-t", $Tag
    "-f", "Dockerfile"
    "--progress=plain"
)

if ($NoBuildCache) {
    $dockerArgs += "--no-cache"
}

# Build context (don't use --build-context for vcpkg-cache as it copies 26GB!)
# Instead, vcpkg will compile from source which is actually faster
$dockerArgs += $tempBuildContext

Write-Host "`n==========================================" -ForegroundColor Green
Write-Host "Starting Docker build..." -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Green
Write-Host "Command: docker $($dockerArgs -join ' ')" -ForegroundColor DarkGray
Write-Host "Log: $logFile" -ForegroundColor DarkGray
Write-Host ""

# Execute build  
try {
    & docker @dockerArgs 2>&1 | Tee-Object -FilePath $logFile
    
    $exitCode = $LASTEXITCODE
    
    if ($exitCode -eq 0) {
        Write-Host "`n✓ Docker build successful" -ForegroundColor Green
        Write-Host "`nImage: $Tag" -ForegroundColor Cyan
        docker images $Tag
    } else {
        Write-Host "`n✗ Docker build failed" -ForegroundColor Red
        Write-Host "Check log: $logFile" -ForegroundColor Yellow
        exit $exitCode
    }
} finally {
    Write-Host "`nCleaning up build context..." -ForegroundColor Yellow
    if (Test-Path $tempBuildContext) {
        Remove-Item $tempBuildContext -Recurse -Force -ErrorAction SilentlyContinue
    }
    Write-Host "✓ Cleanup complete" -ForegroundColor Green
}

Write-Host "`n==========================================" -ForegroundColor Cyan
Write-Host "Build complete!" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
