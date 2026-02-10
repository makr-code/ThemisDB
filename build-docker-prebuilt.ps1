#!/usr/bin/env pwsh
# ============================================================================
# Docker Build Script with Hybrid Pre-built + Delta Install
# ============================================================================
# Strategy:
#   1. Copy pre-built vcpkg_installed from WSL/Linux build
#   2. Docker build validates and installs only missing packages
#   3. Combines speed (pre-built) with completeness (delta install)
# Reduces build time from ~30min to ~5-10min depending on deltas

param(
    [ValidateSet('MINIMAL', 'COMMUNITY', 'ENTERPRISE', 'HYPERSCALER')]
    [string]$Edition = 'COMMUNITY',
    
    [switch]$EnableLLM,
    [switch]$EnableGPU,
    [switch]$Push,
    [switch]$SkipValidation,
    [string]$Registry = 'docker.io',
    [string]$Namespace = 'themisdb'
)

$ErrorActionPreference = 'Stop'

# ============================================================================
# Validate Pre-built Artifacts
# ============================================================================
$prebuiltPath = "C:\VCC\themis\build-linux\vcpkg_installed\x64-linux"
$usePrebuilt = $false

if (Test-Path $prebuiltPath) {
    $artifactSize = (Get-ChildItem $prebuiltPath -Recurse -File | Measure-Object Length -Sum).Sum / 1MB
    $libCount = (Get-ChildItem "$prebuiltPath\lib" -Filter "*.a" -ErrorAction SilentlyContinue | Measure-Object).Count
    
    Write-Host "✓ Found pre-built artifacts:" -ForegroundColor Green
    Write-Host "  Size: $([math]::Round($artifactSize, 2)) MB" -ForegroundColor Gray
    Write-Host "  Libraries: $libCount static libs" -ForegroundColor Gray
    
    if (-not $SkipValidation) {
        Write-Host "`n🔍 Validating pre-built artifacts..." -ForegroundColor Cyan
        
        # Check critical libraries exist
        $requiredLibs = @('librocksdb.a', 'libboost_system.a', 'libprotobuf.a')
        $missing = @()
        
        foreach ($lib in $requiredLibs) {
            if (-not (Test-Path "$prebuiltPath\lib\$lib")) {
                $missing += $lib
            }
        }
        
        if ($missing.Count -gt 0) {
            Write-Host "⚠️  WARNING: Missing critical libraries:" -ForegroundColor Yellow
            $missing | ForEach-Object { Write-Host "  - $_" -ForegroundColor Yellow }
            Write-Host "  Delta install will build these packages" -ForegroundColor Gray
        } else {
            Write-Host "✓ All critical libraries present" -ForegroundColor Green
        }
    }
    
    $usePrebuilt = $true
} else {
    Write-Host "⚠️  Pre-built artifacts not found at $prebuiltPath" -ForegroundColor Yellow
    Write-Host "   Using full vcpkg install (slower)" -ForegroundColor Gray
}

# ============================================================================
# Build Docker Image with Hybrid Strategy
# ============================================================================
$imageTag = "$Registry/$Namespace/themis:$($Edition.ToLower())"
if ($EnableLLM) { $imageTag += "-llm" }
if ($EnableGPU) { $imageTag += "-gpu" }

$strategy = if ($usePrebuilt) { "Hybrid (Pre-built + Delta)" } else { "Full Install" }
Write-Host "`n🐳 Building Docker image..." -ForegroundColor Cyan
Write-Host "   Edition: $Edition"
Write-Host "   Strategy: $strategy" -ForegroundColor $(if ($usePrebuilt) { 'Green' } else { 'Yellow' })
Write-Host "   LLM: $EnableLLM, GPU: $EnableGPU"
Write-Host "   Tag: $imageTag`n"

$buildArgs = @(
    'buildx', 'build',
    '-f', 'Dockerfile',
    '-t', $imageTag,
    '--build-arg', "THEMIS_EDITION=$Edition",
    '--progress=plain'
)

if ($usePrebuilt) {
    $buildArgs += '--build-arg', 'USE_PREBUILT=ON'
}

if ($EnableLLM) { $buildArgs += '--build-arg', 'ENABLE_LLM=ON' }
if ($EnableGPU) { $buildArgs += '--build-arg', 'ENABLE_GPU=ON' }

Write-Host "Command: docker $($buildArgs -join ' ') .`n" -ForegroundColor Gray

# ============================================================================
# Phase 1: Build deps stage UP TO marker (before vcpkg install)
# ============================================================================
if ($usePrebuilt) {
    Write-Host "[Phase 1/3] Building deps stage (pre-install)..." -ForegroundColor Cyan
    
    # Build first RUN layer only (creates directory structure + marker)
    $depsArgs = $buildArgs + @('--target', 'deps', '-t', "$imageTag-deps-preinstall", '.')
    & docker $depsArgs 2>&1 | Out-Null
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "`n❌ Deps pre-install stage failed" -ForegroundColor Red
        exit 1
    }
    
    # ============================================================================
    # Phase 2: Inject pre-built artifacts BEFORE vcpkg install runs
    # ============================================================================
    Write-Host "`n[Phase 2/3] Injecting pre-built artifacts..." -ForegroundColor Cyan
    
    # Create temporary container
    $containerId = docker create --name themis-hybrid-temp "$imageTag-deps-preinstall"
    
    try {
        Write-Host "Copying x64-linux artifacts (4.2GB)..." -ForegroundColor Gray
        $copyStart = Get-Date
        
        # Copy the entire x64-linux directory
        docker cp "$prebuiltPath" "${containerId}:/build/vcpkg_installed/"
        
        $copyTime = ((Get-Date) - $copyStart).TotalSeconds
        Write-Host "✓ Copied in $([math]::Round($copyTime, 1))s" -ForegroundColor Green
        
        # Verify copy
        $libCount = docker exec $containerId find /build/vcpkg_installed/x64-linux/lib -name '*.a' 2>$null | Measure-Object | Select-Object -ExpandProperty Count
        Write-Host "  Verified: $libCount static libraries" -ForegroundColor Gray
        
        # Commit the container with pre-built artifacts
        Write-Host "Committing modified stage..." -ForegroundColor Gray
        $null = docker commit $containerId "$imageTag-deps-with-prebuilt"
        
        Write-Host "✓ Pre-built artifacts ready for delta install" -ForegroundColor Green
        
    } finally {
        docker rm $containerId -f 2>$null | Out-Null
        docker rmi "$imageTag-deps-preinstall" -f 2>$null | Out-Null
    }
    
    # ============================================================================
    # Phase 3: Resume build from modified stage (vcpkg will only install deltas)
    # ============================================================================
    Write-Host "`n[Phase 3/3] Resuming build with delta install..." -ForegroundColor Cyan
    
    # Continue build from deps stage with pre-built artifacts
    # vcpkg install will run now and skip already-present packages
    $finalArgs = @(
        'buildx', 'build',
        '-f', 'Dockerfile',
        '-t', $imageTag,
        '--build-arg', "THEMIS_EDITION=$Edition",
        '--build-arg', 'USE_PREBUILT=ON',
        '--build-context', "deps-with-prebuilt=docker-image://$imageTag-deps-with-prebuilt",
        '--progress=plain'
    )
    
    if ($EnableLLM) { $finalArgs += '--build-arg', 'ENABLE_LLM=ON' }
    if ($EnableGPU) { $finalArgs += '--build-arg', 'ENABLE_GPU=ON' }
    
    $finalArgs += '.'
    
    & docker $finalArgs
    
    # Cleanup
    docker rmi "$imageTag-deps-with-prebuilt" -f 2>$null | Out-Null
    
} else {
    # Standard build without pre-built artifacts
    Write-Host "[Phase 1/1] Full build..." -ForegroundColor Cyan
    $buildArgs += '.'
    & docker $buildArgs
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "`n❌ Build failed" -ForegroundColor Red
    exit 1
}

Write-Host "`n✓ Build successful: $imageTag" -ForegroundColor Green

# ============================================================================
# Push to Registry (optional)
# ============================================================================
if ($Push) {
    Write-Host "`n📤 Pushing to $Registry..." -ForegroundColor Cyan
    docker push $imageTag
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Pushed: $imageTag" -ForegroundColor Green
    } else {
        Write-Host "❌ Push failed" -ForegroundColor Red
        exit 1
    }
}

Write-Host "`n🎉 Done!" -ForegroundColor Green
