# ThemisDB Release Archive Creation Script
# Creates platform-specific release archives

param(
    [string]$Version = "1.3.4",
    [string]$Platform = "windows-x64",
    [string]$BuildDir = "build-msvc\Release",
    [string]$OutputDir = "release",
    [switch]$IncludeBenchmarks = $false
)

$ErrorActionPreference = "Stop"

Write-Host "Creating ThemisDB v$Version Release Package..." -ForegroundColor Cyan
Write-Host ""

# Create output directory structure
$releaseRoot = Join-Path $OutputDir "themis-v$Version-$Platform"
$releaseBin = Join-Path $releaseRoot "bin"
$releaseDocs = Join-Path $releaseRoot "docs"
$releaseConfig = Join-Path $releaseRoot "config"

Write-Host "Creating directory structure..." -ForegroundColor Green
@($releaseRoot, $releaseBin, $releaseDocs, $releaseConfig) | ForEach-Object {
    if (Test-Path $_) {
        Remove-Item -Path $_ -Recurse -Force
    }
    New-Item -ItemType Directory -Path $_ -Force | Out-Null
}

# Copy binaries
Write-Host "Copying binaries..." -ForegroundColor Green

$binaries = @(
    "themis_server.exe",
    "themis_cli.exe",
    "themis_demo.exe"
)

foreach ($bin in $binaries) {
    $srcPath = Join-Path $BuildDir $bin
    if (Test-Path $srcPath) {
        Copy-Item -Path $srcPath -Destination $releaseBin -Force
        Write-Host "  + $bin" -ForegroundColor Gray
    }
}

# Copy DLLs
$dlls = Get-ChildItem -Path $BuildDir -Filter "*.dll" -ErrorAction SilentlyContinue
if ($dlls) {
    $dlls | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination $releaseBin -Force
    }
    Write-Host "  + $($dlls.Count) DLL files" -ForegroundColor Gray
}

# Copy benchmarks if requested
if ($IncludeBenchmarks) {
    Write-Host "Copying benchmarks..." -ForegroundColor Green
    $benchmarkDir = Join-Path $releaseBin "benchmarks"
    New-Item -ItemType Directory -Path $benchmarkDir -Force | Out-Null
    
    $benchmarks = @(
        "bench_v1_3_4_optimizations.exe",
        "bench_batch_insert.exe"
    )
    
    foreach ($bench in $benchmarks) {
        $srcPath = Join-Path $BuildDir $bench
        if (Test-Path $srcPath) {
            Copy-Item -Path $srcPath -Destination $benchmarkDir -Force
            Write-Host "  + $bench" -ForegroundColor Gray
        }
    }
}

# Copy documentation
Write-Host "Copying documentation..." -ForegroundColor Green

$docFiles = @(
    @{Src = "README.md"; Dst = "README.md"},
    @{Src = "CHANGELOG.md"; Dst = "CHANGELOG.md"},
    @{Src = "RELEASE_NOTES_v$Version.md"; Dst = "RELEASE_NOTES.md"},
    @{Src = "BATCH_INSERT_PERFORMANCE_RESULTS.md"; Dst = "PERFORMANCE.md"},
    @{Src = "LICENSE"; Dst = "LICENSE"}
)

foreach ($doc in $docFiles) {
    if (Test-Path $doc.Src) {
        Copy-Item -Path $doc.Src -Destination (Join-Path $releaseDocs $doc.Dst) -Force
        Write-Host "  + $($doc.Dst)" -ForegroundColor Gray
    }
}

# Copy config files
Write-Host "Copying configuration..." -ForegroundColor Green
if (Test-Path "config") {
    Copy-Item -Path "config\*" -Destination $releaseConfig -Recurse -Force -ErrorAction SilentlyContinue
}

# Create startup script
Write-Host "Creating startup script..." -ForegroundColor Green

$startScript = @"
@echo off
echo Starting ThemisDB v$Version...
echo.

cd /d "%~dp0"

if not exist "data" (
    echo Creating data directory...
    mkdir data
)

bin\themis_server.exe --help

echo.
echo To start the server, run:
echo   bin\themis_server.exe
"@

$startScript | Out-File -FilePath (Join-Path $releaseRoot "start.bat") -Encoding ASCII

# Create README
$readme = @"
ThemisDB v$Version - $Platform

Quick Start
-----------
1. Run start.bat to see usage information
2. Edit config files in config/ folder
3. Start server: bin\themis_server.exe

Documentation
-------------
See docs/ folder for complete documentation

What's New
----------
- 23-77x faster bulk inserts
- Metadata caching
- Enhanced performance

For full details, see docs/RELEASE_NOTES.md

System Requirements
-------------------
- Windows 10/11 or Server 2019+
- 4 GB RAM minimum
- Visual C++ Redistributable 2022

Support
-------
GitHub: https://github.com/makr-code/ThemisDB
"@

$readme | Out-File -FilePath (Join-Path $releaseRoot "README.txt") -Encoding UTF8

# Create ZIP archive
Write-Host "Creating ZIP archive..." -ForegroundColor Green

$zipFile = Join-Path $OutputDir "themis-v$Version-$Platform.zip"
if (Test-Path $zipFile) {
    Remove-Item $zipFile -Force
}

Compress-Archive -Path $releaseRoot -DestinationPath $zipFile -CompressionLevel Optimal

# Generate checksum
Write-Host "Generating checksum..." -ForegroundColor Green

$hash = Get-FileHash -Path $zipFile -Algorithm SHA256
$checksumFile = Join-Path $OutputDir "themis-v$Version-$Platform.sha256"
"$($hash.Hash.ToLower())  themis-v$Version-$Platform.zip" | Out-File -FilePath $checksumFile -Encoding ASCII

# Summary
$zipSize = (Get-Item $zipFile).Length / 1MB
Write-Host ""
Write-Host "Release package created successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "Archive: $zipFile" -ForegroundColor Cyan
Write-Host "Size: $([math]::Round($zipSize, 2)) MB" -ForegroundColor Cyan
Write-Host "SHA256: $checksumFile" -ForegroundColor Cyan
Write-Host "Hash: $($hash.Hash.ToLower())" -ForegroundColor Gray
Write-Host ""
Write-Host "Next: Upload to GitHub Release" -ForegroundColor Yellow
