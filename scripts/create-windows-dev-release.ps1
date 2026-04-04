# ThemisDB Windows Dev Release Creation Script
# Creates a Windows development release package and uploads to GitHub

param(
    [string]$Version = "",
    [string]$BuildDir = "build-msvc-ninja-release",
    [string]$OutputDir = "releases",
    [string]$GitHubToken = $env:GITHUB_TOKEN,
    [switch]$SkipUpload = $false
)

$ErrorActionPreference = "Stop"

# Read version from VERSION file if not specified
if ([string]::IsNullOrEmpty($Version)) {
    if (Test-Path "VERSION") {
        $Version = (Get-Content "VERSION" -Raw).Trim()
        Write-Host "Using version from VERSION file: $Version" -ForegroundColor Cyan
    } else {
        Write-Error "No version specified and VERSION file not found"
        exit 1
    }
}

# Read release type from RELEASE_TYPE file
$ReleaseType = "alpha"
if (Test-Path "RELEASE_TYPE") {
    $ReleaseType = (Get-Content "RELEASE_TYPE" -Raw).Trim()
}

$Platform = "windows-x64-dev"
$FullVersion = "$Version-$ReleaseType"

Write-Host "═══════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "  ThemisDB Windows Dev Release Package" -ForegroundColor White
Write-Host "═══════════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "Version:      $FullVersion" -ForegroundColor Yellow
Write-Host "Platform:     $Platform" -ForegroundColor Yellow
Write-Host "Build Dir:    $BuildDir" -ForegroundColor Yellow
Write-Host "Output Dir:   $OutputDir" -ForegroundColor Yellow
Write-Host ""

# Verify build directory exists
if (-not (Test-Path $BuildDir)) {
    Write-Error "Build directory not found: $BuildDir"
    exit 1
}

# Create output directory structure
$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$releaseRoot = Join-Path $OutputDir "themisdb-$FullVersion-$Platform"
$releaseBin = Join-Path $releaseRoot "bin"
$releaseDocs = Join-Path $releaseRoot "docs"
$releaseConfig = Join-Path $releaseRoot "config"
$releaseTests = Join-Path $releaseRoot "tests"
$releaseLib = Join-Path $releaseRoot "lib"

Write-Host "Creating directory structure..." -ForegroundColor Green
@($OutputDir, $releaseRoot, $releaseBin, $releaseDocs, $releaseConfig, $releaseTests, $releaseLib) | ForEach-Object {
    if (-not (Test-Path $_)) {
        New-Item -ItemType Directory -Path $_ -Force | Out-Null
    }
}

# Copy main binaries
Write-Host "Copying binaries..." -ForegroundColor Green

$mainBinaries = @(
    @{Name = "themis_server.exe"; Path = "themis_server.exe"}
)

foreach ($bin in $mainBinaries) {
    $srcPath = Join-Path $BuildDir $bin.Path
    if (Test-Path $srcPath) {
        Copy-Item -Path $srcPath -Destination $releaseBin -Force
        Write-Host "  ✓ $($bin.Name)" -ForegroundColor Gray
    } else {
        Write-Host "  ⚠ $($bin.Name) not found at $srcPath" -ForegroundColor Yellow
    }
}

# Copy test and benchmark binaries
Write-Host "Copying test and benchmark binaries..." -ForegroundColor Green

$testBinary = Join-Path $BuildDir "cmake\tests\themis_tests.exe"
if (Test-Path $testBinary) {
    Copy-Item -Path $testBinary -Destination $releaseTests -Force
    Write-Host "  ✓ themis_tests.exe" -ForegroundColor Gray
} else {
    Write-Host "  ⚠ themis_tests.exe not found" -ForegroundColor Yellow
}

$benchmarkBinary = Join-Path $BuildDir "cmake\themis_benchmarks.exe"
if (Test-Path $benchmarkBinary) {
    Copy-Item -Path $benchmarkBinary -Destination $releaseTests -Force
    Write-Host "  ✓ themis_benchmarks.exe" -ForegroundColor Gray
} else {
    Write-Host "  ⚠ themis_benchmarks.exe not found" -ForegroundColor Yellow
}

# Copy core library
Write-Host "Copying libraries..." -ForegroundColor Green
$coreLib = Join-Path $BuildDir "cmake\themis_core.lib"
if (Test-Path $coreLib) {
    Copy-Item -Path $coreLib -Destination $releaseLib -Force
    Write-Host "  ✓ themis_core.lib" -ForegroundColor Gray
}

# Copy DLLs from vcpkg
Write-Host "Copying runtime dependencies (DLLs)..." -ForegroundColor Green
$vcpkgBinDir = Join-Path $BuildDir "vcpkg_installed\x64-windows\bin"
if (Test-Path $vcpkgBinDir) {
    $dlls = Get-ChildItem -Path $vcpkgBinDir -Filter "*.dll"
    $dllCount = 0
    foreach ($dll in $dlls) {
        Copy-Item -Path $dll.FullName -Destination $releaseBin -Force
        $dllCount++
    }
    Write-Host "  ✓ Copied $dllCount DLLs from vcpkg" -ForegroundColor Gray
    
    # Also copy to tests directory
    foreach ($dll in $dlls) {
        Copy-Item -Path $dll.FullName -Destination $releaseTests -Force
    }
    Write-Host "  ✓ Copied DLLs to tests directory" -ForegroundColor Gray
}

# Copy llama.cpp DLLs if they exist
$llamaBuildDir = Join-Path $BuildDir "llama_cpp_build"
if (Test-Path $llamaBuildDir) {
    $llamaDlls = Get-ChildItem -Path $llamaBuildDir -Filter "*.dll" -Recurse
    foreach ($dll in $llamaDlls) {
        Copy-Item -Path $dll.FullName -Destination $releaseBin -Force
        Copy-Item -Path $dll.FullName -Destination $releaseTests -Force
    }
    Write-Host "  ✓ Copied llama.cpp DLLs" -ForegroundColor Gray
}

# Copy configuration files
Write-Host "Copying configuration files..." -ForegroundColor Green
if (Test-Path "config") {
    $configFiles = Get-ChildItem -Path "config" -File
    foreach ($file in $configFiles) {
        Copy-Item -Path $file.FullName -Destination $releaseConfig -Force
        Write-Host "  ✓ $($file.Name)" -ForegroundColor Gray
    }
}

# Copy documentation
Write-Host "Copying documentation..." -ForegroundColor Green

$docFiles = @(
    "README.md",
    "CHANGELOG.md",
    "LICENSE",
    "QUICKSTART.md",
    "SECURITY.md",
    "CONTRIBUTING.md"
)

foreach ($doc in $docFiles) {
    if (Test-Path $doc) {
        Copy-Item -Path $doc -Destination $releaseDocs -Force
        Write-Host "  ✓ $doc" -ForegroundColor Gray
    }
}

# Copy latest release notes if they exist
$releaseNotesPattern = "docs\de\releases\RELEASE_NOTES_v$($Version.Split('-')[0])*.md"
$releaseNotes = Get-ChildItem -Path $releaseNotesPattern -ErrorAction SilentlyContinue | Select-Object -First 1
if ($releaseNotes) {
    Copy-Item -Path $releaseNotes.FullName -Destination (Join-Path $releaseDocs "RELEASE_NOTES.md") -Force
    Write-Host "  ✓ RELEASE_NOTES.md" -ForegroundColor Gray
}

# Create README for the release
$releaseReadme = @"
# ThemisDB $FullVersion - Windows Development Release

## 📦 Package Contents

- **bin/**: Main executables and runtime DLLs
  - themis_server.exe - ThemisDB server
  - All required runtime dependencies (DLLs)

- **tests/**: Test suite
  - themis_tests.exe - Comprehensive test suite (488+ tests)
  - All test runtime dependencies

- **lib/**: Static libraries
  - themis_core.lib - Core library for development

- **config/**: Configuration files
  - Default configuration templates
  - Example configurations for various scenarios

- **docs/**: Documentation
  - README, CHANGELOG, License
  - Release notes and guides

## 🚀 Quick Start

### Running the Server

``````powershell
cd bin
.\themis_server.exe --help
``````

### Running Tests

``````powershell
cd tests
.\themis_tests.exe
``````

## 📋 System Requirements

- **OS**: Windows 10/11 or Windows Server 2019+
- **RAM**: 4 GB minimum, 8 GB recommended
- **Disk**: 2 GB free space
- **CPU**: x64 processor with AVX2 support

## 🔧 Build Information

- **Version**: $FullVersion
- **Platform**: Windows x64
- **Build Type**: Release (with optimizations)
- **Compiler**: MSVC 19.44
- **Build System**: CMake + Ninja
- **Features**:
  - ✓ LLM Support (llama.cpp integration)
  - ✓ gRPC Support
  - ✓ HTTP Server
  - ✓ RocksDB Backend
  - ✓ Vector Search (HNSW)
  - ✓ AVX2 Optimizations
  - ✓ Performance Optimizations (RCU Index, LIRS Cache)
  - ✓ mimalloc Allocator

## 🐛 Known Issues

This is a **development release** intended for testing and development purposes.

For production use, please wait for a stable release.

## 📄 License

MIT License - See LICENSE file for details

## 🔗 Links

- GitHub: https://github.com/kyr0/themis
- Documentation: https://themisdb.org/docs
- Issue Tracker: https://github.com/kyr0/themis/issues

## 📝 What's New in v$Version

See docs/RELEASE_NOTES.md for detailed release notes.

---
Generated: $timestamp
"@

$releaseReadme | Out-File -FilePath (Join-Path $releaseRoot "README.md") -Encoding UTF8

# Create Windows startup script
$startScript = @"
@echo off
echo ══════════════════════════════════════════
echo   ThemisDB $FullVersion
echo ══════════════════════════════════════════
echo.

cd /d "%~dp0bin"

if not exist "..\data" (
    echo Creating data directory...
    mkdir "..\data"
)

echo Starting ThemisDB Server...
echo.

themis_server.exe %*
"@

$startScript | Out-File -FilePath (Join-Path $releaseRoot "start-server.bat") -Encoding ASCII

# Create test runner script
$testScript = @"
@echo off
echo ══════════════════════════════════════════
echo   ThemisDB Test Suite
echo ══════════════════════════════════════════
echo.

cd /d "%~dp0tests"

echo Running tests...
echo.

themis_tests.exe %*

echo.
pause
"@

$testScript | Out-File -FilePath (Join-Path $releaseRoot "run-tests.bat") -Encoding ASCII

# Generate SHA256 checksums
Write-Host "Generating checksums..." -ForegroundColor Green
$checksumFile = Join-Path $releaseRoot "SHA256SUMS.txt"
$checksums = @()

Get-ChildItem -Path $releaseRoot -Recurse -File | Where-Object { $_.Name -ne "SHA256SUMS.txt" } | ForEach-Object {
    $hash = (Get-FileHash -Path $_.FullName -Algorithm SHA256).Hash
    $relativePath = $_.FullName.Substring($releaseRoot.Length + 1)
    $checksums += "$hash  $relativePath"
}

$checksums | Out-File -FilePath $checksumFile -Encoding ASCII
Write-Host "  ✓ SHA256SUMS.txt created" -ForegroundColor Gray

# Create ZIP archive (using 7z if available, otherwise tar.gz)
Write-Host "Creating archive..." -ForegroundColor Green
$archiveName = "themisdb-$FullVersion-$Platform"
$archivePath = ""

# Try 7z first (better for large files)
try {
    $7zPath = Get-Command "7z.exe" -ErrorAction SilentlyContinue
    if ($7zPath) {
        $archiveName += ".7z"
        $archivePath = Join-Path $OutputDir $archiveName
        
        if (Test-Path $archivePath) {
            Remove-Item $archivePath -Force
        }
        
        & 7z.exe a -t7z -mx=5 $archivePath $releaseRoot | Out-Null
        Write-Host "  ✓ Created $archiveName (7z format)" -ForegroundColor Gray
    }
} catch {
    $7zPath = $null
}

# Fallback to tar.gz if 7z not available
if (-not $7zPath) {
    $archiveName += ".tar.gz"
    $archivePath = Join-Path $OutputDir $archiveName
    
    if (Test-Path $archivePath) {
        Remove-Item $archivePath -Force
    }
    
    # Create tar.gz using PowerShell (Windows 10+)
    $tarFile = Join-Path $OutputDir "themisdb-$FullVersion-$Platform.tar"
    tar -czf $archivePath -C $OutputDir (Split-Path $releaseRoot -Leaf)
    
    Write-Host "  ✓ Created $archiveName (tar.gz format)" -ForegroundColor Gray
}

$archiveSize = (Get-Item $archivePath).Length / 1MB
Write-Host "  ✓ Size: $([math]::Round($archiveSize, 2)) MB" -ForegroundColor Gray

# Calculate archive checksum
$zipHash = (Get-FileHash -Path $archivePath -Algorithm SHA256).Hash
Write-Host "  ✓ SHA256: $zipHash" -ForegroundColor Gray

$zipPath = $archivePath
$zipName = $archiveName
$zipSize = $archiveSize

Write-Host ""
Write-Host "═══════════════════════════════════════════════════════" -ForegroundColor Green
Write-Host "  Release package created successfully!" -ForegroundColor White
Write-Host "═══════════════════════════════════════════════════════" -ForegroundColor Green
Write-Host ""
Write-Host "Package: $zipPath" -ForegroundColor Cyan
Write-Host "Size:    $([math]::Round($zipSize, 2)) MB" -ForegroundColor Cyan
Write-Host ""

# Upload to GitHub if requested
if (-not $SkipUpload) {
    Write-Host "Uploading to GitHub..." -ForegroundColor Yellow
    
    if ([string]::IsNullOrEmpty($GitHubToken)) {
        Write-Host "⚠ No GitHub token provided. Skipping upload." -ForegroundColor Yellow
        Write-Host "  Set GITHUB_TOKEN environment variable or use -GitHubToken parameter" -ForegroundColor Gray
        Write-Host ""
        Write-Host "To upload manually:" -ForegroundColor Cyan
        Write-Host "  gh release create v$FullVersion $zipPath --title 'v$FullVersion' --notes 'Windows Development Release'" -ForegroundColor Gray
    } else {
        # Check if gh CLI is available
        try {
            $ghVersion = gh --version 2>&1
            Write-Host "  ✓ GitHub CLI detected" -ForegroundColor Gray
            
            # Check if release exists
            $releaseExists = $false
            try {
                gh release view "v$FullVersion" 2>&1 | Out-Null
                $releaseExists = $true
            } catch {
                $releaseExists = $false
            }
            
            if ($releaseExists) {
                Write-Host "  ✓ Release v$FullVersion already exists" -ForegroundColor Gray
                Write-Host "  Uploading asset..." -ForegroundColor Gray
                
                gh release upload "v$FullVersion" $zipPath --clobber
                Write-Host "  ✓ Asset uploaded successfully" -ForegroundColor Green
            } else {
                Write-Host "  Creating new release v$FullVersion..." -ForegroundColor Gray
                
                $releaseNotes = @"
# ThemisDB $FullVersion - Windows Development Release

Windows development build with full test suite.

## Features
- ✓ LLM Support (llama.cpp)
- ✓ gRPC Support
- ✓ HTTP Server
- ✓ Vector Search (HNSW)
- ✓ Performance Optimizations

## Package Contents
- themis_server.exe - Main server
- themis_tests.exe - Test suite (488+ tests)
- All runtime dependencies (DLLs)
- Configuration templates
- Documentation

## System Requirements
- Windows 10/11 or Server 2019+
- 4 GB RAM minimum
- x64 processor with AVX2 support

**SHA256**: ``$zipHash``
"@
                
                $releaseNotesFile = Join-Path $env:TEMP "release-notes-$timestamp.md"
                $releaseNotes | Out-File -FilePath $releaseNotesFile -Encoding UTF8
                
                gh release create "v$FullVersion" $zipPath `
                    --title "v$FullVersion" `
                    --notes-file $releaseNotesFile `
                    --prerelease
                
                Remove-Item $releaseNotesFile -Force
                Write-Host "  ✓ Release created and asset uploaded" -ForegroundColor Green
            }
            
            Write-Host ""
            Write-Host "═══════════════════════════════════════════════════════" -ForegroundColor Green
            Write-Host "  GitHub Release Published!" -ForegroundColor White
            Write-Host "═══════════════════════════════════════════════════════" -ForegroundColor Green
            Write-Host ""
            Write-Host "View at: https://github.com/kyr0/themis/releases/tag/v$FullVersion" -ForegroundColor Cyan
            
        } catch {
            Write-Host "  ✗ GitHub CLI not found" -ForegroundColor Red
            Write-Host "  Install from: https://cli.github.com/" -ForegroundColor Gray
            Write-Host ""
            Write-Host "Or upload manually with:" -ForegroundColor Cyan
            Write-Host "  gh release create v$FullVersion $zipPath --title 'v$FullVersion' --prerelease" -ForegroundColor Gray
        }
    }
}

Write-Host ""
Write-Host "Done! 🎉" -ForegroundColor Green
