#!/usr/bin/env pwsh
# setup-vcpkg-offline.ps1
# ThemisDB vcpkg Offline Cache Setup
# Lädt alle Dependencies herunter für offline builds

param(
    [string[]]$Triplets = @("x64-windows", "x64-linux", "arm64-linux"),
    [switch]$SkipBootstrap,
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir
$VcpkgRoot = Join-Path $RootDir "vcpkg"
$VcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
$DownloadsDir = Join-Path $VcpkgRoot "downloads"

Write-Host "🚀 ThemisDB vcpkg Offline Cache Setup" -ForegroundColor Cyan
Write-Host "═══════════════════════════════════════`n" -ForegroundColor Cyan

# 1. vcpkg Bootstrap
if (-not $SkipBootstrap) {
    Write-Host "📦 Step 1/4: vcpkg Bootstrap" -ForegroundColor Yellow
    
    if (-not (Test-Path $VcpkgExe)) {
        Write-Host "  ⚙️  Bootstrapping vcpkg..." -ForegroundColor Gray
        Push-Location $VcpkgRoot
        .\bootstrap-vcpkg.bat -disableMetrics
        Pop-Location
        Write-Host "  ✅ vcpkg bootstrapped" -ForegroundColor Green
    } else {
        Write-Host "  ✅ vcpkg already bootstrapped" -ForegroundColor Green
    }
} else {
    Write-Host "📦 Step 1/4: vcpkg Bootstrap (skipped)" -ForegroundColor Gray
}

# 2. vcpkg Update
Write-Host "`n📦 Step 2/4: vcpkg Repository Update" -ForegroundColor Yellow
Write-Host "  ⚙️  Pulling latest vcpkg registry..." -ForegroundColor Gray

Push-Location $VcpkgRoot
$gitOutput = git pull 2>&1
Pop-Location

if ($LASTEXITCODE -eq 0) {
    Write-Host "  ✅ vcpkg registry updated" -ForegroundColor Green
} else {
    Write-Host "  ⚠️  vcpkg update failed (continuing anyway)" -ForegroundColor Yellow
}

# 3. Download Dependencies
Write-Host "`n📦 Step 3/4: Download Source Archives" -ForegroundColor Yellow

# Dependencies from vcpkg.json
$CoreDependencies = @(
    "openssl",
    "rocksdb[lz4,zstd]",
    "simdjson",
    "tbb",
    "arrow[parquet,compute]",
    "hnswlib",
    "gtest",
    "benchmark",
    "boost-asio",
    "boost-beast",
    "spdlog",
    "nlohmann-json",
    "opentelemetry-cpp[otlp-http]",
    "curl",
    "yaml-cpp",
    "zstd",
    "mimalloc"
)

$OptionalDependencies = @(
    "faiss",      # GPU feature
    "grpc",       # RPC feature
    "protobuf"    # RPC feature
)

$AllDependencies = $CoreDependencies + $OptionalDependencies

foreach ($triplet in $Triplets) {
    Write-Host "  📥 Downloading for triplet: $triplet" -ForegroundColor Cyan
    
    foreach ($dep in $AllDependencies) {
        if ($Verbose) {
            Write-Host "    - $dep" -ForegroundColor Gray
        }
        
        $downloadArgs = @(
            "x-download",
            $dep,
            "--triplet", $triplet
        )
        
        & $VcpkgExe $downloadArgs 2>&1 | Out-Null
        
        if ($LASTEXITCODE -ne 0 -and $Verbose) {
            Write-Host "      ⚠️  Warning: $dep download failed" -ForegroundColor Yellow
        }
    }
    
    Write-Host "  ✅ Triplet $triplet complete" -ForegroundColor Green
}

# 4. Verify Cache
Write-Host "`n📦 Step 4/4: Verify Cache" -ForegroundColor Yellow

if (Test-Path $DownloadsDir) {
    $archiveCount = (Get-ChildItem -Path $DownloadsDir -File).Count
    $cacheSize = (Get-ChildItem -Path $DownloadsDir -File -Recurse | Measure-Object -Property Length -Sum).Sum / 1GB
    
    Write-Host "  ✅ Cache ready:" -ForegroundColor Green
    Write-Host "     - Archives: $archiveCount" -ForegroundColor Gray
    Write-Host "     - Size: $([math]::Round($cacheSize, 2)) GB" -ForegroundColor Gray
    Write-Host "     - Location: $DownloadsDir" -ForegroundColor Gray
} else {
    Write-Host "  ⚠️  Warning: downloads/ directory not found" -ForegroundColor Yellow
}

Write-Host "`n═══════════════════════════════════════" -ForegroundColor Cyan
Write-Host "✅ vcpkg Offline Cache Setup Complete!" -ForegroundColor Green
Write-Host "`n💡 Next Steps:" -ForegroundColor Cyan
Write-Host "   1. Build: cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake" -ForegroundColor Gray
Write-Host "   2. Compile: cmake --build build --config Release" -ForegroundColor Gray
Write-Host "`n📚 Docs: docs/deployment/VCPKG_OFFLINE_STRATEGY.md`n" -ForegroundColor Gray
