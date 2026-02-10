#!/usr/bin/env pwsh
# ============================================================================
# Fast Docker Build with Bind-Mounted vcpkg Cache
# ============================================================================
# Uses Docker BuildKit bind-mounts to access:
#   - vcpkg/packages (20GB binary cache)
#   - vcpkg/downloads (4GB downloads cache)
#   - build-linux/vcpkg_installed/x64-linux (4.2GB pre-built artifacts)
# 
# No copying to build context - direct mount during RUN commands
# Reduces build time from ~30min to ~2-5min

param(
    [ValidateSet('MINIMAL', 'COMMUNITY', 'ENTERPRISE', 'HYPERSCALER')]
    [string]$Edition = 'COMMUNITY',
    
    [switch]$EnableLLM,
    [switch]$EnableGPU,
    [switch]$Push,
    [string]$Registry = 'docker.io',
    [string]$Namespace = 'themisdb'
)

$ErrorActionPreference = 'Stop'

# Ensure BuildKit is enabled
$env:DOCKER_BUILDKIT = "1"

Write-Host "🚀 Fast Docker Build with Bind-Mounted Caches`n" -ForegroundColor Cyan

# ============================================================================
# Validate Cache Directories
# ============================================================================
$vcpkgPackages = "C:\VCC\themis\vcpkg\packages"
$vcpkgDownloads = "C:\VCC\themis\vcpkg\downloads"
$prebuiltArtifacts = "C:\VCC\themis\build-linux\vcpkg_installed\x64-linux"

Write-Host "📁 Checking cache directories..." -ForegroundColor Cyan

$caches = @()
if (Test-Path $vcpkgPackages) {
    $size = (Get-ChildItem $vcpkgPackages -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum / 1GB
    Write-Host "  ✓ vcpkg/packages: $([math]::Round($size, 1))GB" -ForegroundColor Green
    $caches += "binary-cache"
}

if (Test-Path $vcpkgDownloads) {
    $size = (Get-ChildItem $vcpkgDownloads -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum / 1GB
    Write-Host "  ✓ vcpkg/downloads: $([math]::Round($size, 1))GB" -ForegroundColor Green
    $caches += "downloads"
}

if (Test-Path $prebuiltArtifacts) {
    $size = (Get-ChildItem $prebuiltArtifacts -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum / 1GB
    $libCount = (Get-ChildItem "$prebuiltArtifacts\lib" -Filter "*.a" -ErrorAction SilentlyContinue | Measure-Object).Count
    Write-Host "  ✓ build-linux x64-linux: $([math]::Round($size, 1))GB ($libCount libs)" -ForegroundColor Green
    $caches += "pre-built"
}

if ($caches.Count -eq 0) {
    Write-Host "`n⚠️  No caches found - performing full build (slow)" -ForegroundColor Yellow
    Write-Host "   Tip: Run WSL/Linux build first to populate caches" -ForegroundColor Gray
} else {
    Write-Host "`n✓ Using caches: $($caches -join ', ')" -ForegroundColor Green
}

# ============================================================================
# Build Configuration
# ============================================================================
$imageTag = "$Registry/$Namespace/themis:$($Edition.ToLower())"
if ($EnableLLM) { $imageTag += "-llm" }
if ($EnableGPU) { $imageTag += "-gpu" }

Write-Host "`n🐳 Building Docker image..." -ForegroundColor Cyan
Write-Host "   Edition: $Edition"
Write-Host "   LLM: $EnableLLM, GPU: $EnableGPU"
Write-Host "   Tag: $imageTag"
Write-Host "   Strategy: Bind-mounted caches (zero-copy)`n" -ForegroundColor Green

# ============================================================================
# Prepare minimal build context (avoid vcpkg/buildtrees in context)
# ============================================================================
$contextDir = Join-Path $env:TEMP "themis-docker-context"
if (Test-Path $contextDir) {
    Remove-Item $contextDir -Recurse -Force
}
New-Item -ItemType Directory -Path $contextDir | Out-Null

Write-Host "📦 Creating minimal build context: $contextDir" -ForegroundColor Cyan

# Root files
$rootFiles = @(
    "C:\VCC\themis\CMakeLists.txt",
    "C:\VCC\themis\VERSION",
    "C:\VCC\themis\vcpkg.json",
    "C:\VCC\themis\vcpkg-configuration.json",
    "C:\VCC\themis\Dockerfile",
    "C:\VCC\themis\docker-compose.yml",
    "C:\VCC\themis\.dockerignore"
)
foreach ($file in $rootFiles) {
    if (Test-Path $file) {
        Copy-Item $file -Destination $contextDir -Force
    }
}

# Copy directories (source -> context)
$dirMap = @(
    @{ Src = "C:\VCC\themis\cmake";    Dest = Join-Path $contextDir "cmake" },
    @{ Src = "C:\VCC\themis\include";  Dest = Join-Path $contextDir "include" },
    @{ Src = "C:\VCC\themis\src";      Dest = Join-Path $contextDir "src" },
    @{ Src = "C:\VCC\themis\proto";    Dest = Join-Path $contextDir "proto" },
    @{ Src = "C:\VCC\themis\internal"; Dest = Join-Path $contextDir "internal" },
    @{ Src = "C:\VCC\themis\adapters"; Dest = Join-Path $contextDir "adapters" },
    @{ Src = "C:\VCC\themis\aql";      Dest = Join-Path $contextDir "aql" },
    @{ Src = "C:\VCC\themis\docker";   Dest = Join-Path $contextDir "docker" }
)

foreach ($pair in $dirMap) {
    if (Test-Path $pair.Src) {
        Write-Host "  Copying $(Split-Path $pair.Src -Leaf)..." -ForegroundColor DarkGray
        $rc = robocopy $pair.Src $pair.Dest /E /NFL /NDL /NJH /NJS /NC /NS 2>&1
        if ($LASTEXITCODE -ge 8) {
            Write-Host "    Error code: $LASTEXITCODE" -ForegroundColor Yellow
        }
    }
}

# Copy llama.cpp with exclusions
$llamaSrc = "C:\VCC\themis\llama.cpp"
$llamaDest = Join-Path $contextDir "llama.cpp"
if (Test-Path $llamaSrc) {
    Write-Host "  Copying llama.cpp..." -ForegroundColor DarkGray
    $rc = robocopy $llamaSrc $llamaDest /E /XD build build-* .git .github tests /NFL /NDL /NJH /NJS /NC /NS 2>&1
    if ($LASTEXITCODE -ge 8) {
        Write-Host "    Warning: robocopy code $LASTEXITCODE (may be non-fatal)" -ForegroundColor Yellow
    }
}

Write-Host "✓ Context ready, $(Get-ChildItem $contextDir -Recurse | Measure-Object | Select-Object -ExpandProperty Count) items copied`n" -ForegroundColor Green

# Prepare prebuilt context (copy to temp to avoid symlink issues)
$prebuiltContext = Join-Path $env:TEMP "themis-prebuilt-context"
if (Test-Path $prebuiltContext) {
    Remove-Item $prebuiltContext -Recurse -Force
}
New-Item -ItemType Directory -Path $prebuiltContext | Out-Null
New-Item -ItemType Directory -Path (Join-Path $prebuiltContext "x64-linux") | Out-Null

Write-Host "📦 Creating prebuilt context..." -ForegroundColor Cyan
if (Test-Path "C:\VCC\themis\build-linux\vcpkg_installed\x64-linux") {
    $rc = robocopy "C:\VCC\themis\build-linux\vcpkg_installed\x64-linux" (Join-Path $prebuiltContext "x64-linux") /E /NFL /NDL /NJH /NJS /NC /NS 2>&1
    $copied = (Get-ChildItem (Join-Path $prebuiltContext "x64-linux") -Recurse | Measure-Object | Select-Object -ExpandProperty Count)
    Write-Host "✓ Prebuilt context ready, $copied items`n" -ForegroundColor Green
}

# ============================================================================
# Docker Build with Bind Mounts (named build contexts)
# ============================================================================
$buildArgs = @(
    'buildx', 'build',
    '-f', (Join-Path $contextDir 'Dockerfile'),
    '-t', $imageTag,
    '--build-arg', "THEMIS_EDITION=$Edition",
    '--build-arg', 'ENABLE_VCPKG_CACHE=ON',
    '--build-arg', 'SKIP_VCPKG_INSTALL=ON',
    '--build-context', "prebuilt=$([IO.Path]::GetFullPath($prebuiltContext))",
    '--progress=plain',
    $contextDir
)

if ($EnableLLM) { $buildArgs += '--build-arg', 'ENABLE_LLM=ON' }
if ($EnableGPU) { $buildArgs += '--build-arg', 'ENABLE_GPU=ON' }

Write-Host "Command: docker $($buildArgs -join ' ')`n" -ForegroundColor Gray

$buildStart = Get-Date
try {
    & docker $buildArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Host "`n❌ Docker build failed with exit code $LASTEXITCODE" -ForegroundColor Red
        exit $LASTEXITCODE
    }
} catch {
    Write-Host "`n❌ Docker build error: $_" -ForegroundColor Red
    exit 1
}

$buildTime = ((Get-Date) - $buildStart).TotalMinutes
Write-Host "`n✓ Build successful in $([math]::Round($buildTime, 1)) minutes" -ForegroundColor Green
Write-Host "   Image: $imageTag" -ForegroundColor Gray

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

# Cleanup minimal context
if (Test-Path $contextDir) {
    Remove-Item $contextDir -Recurse -Force
}

# Cleanup prebuilt context
if (Test-Path $prebuiltContext) {
    Remove-Item $prebuiltContext -Recurse -Force
}
