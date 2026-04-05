# ThemisDB Release Archive Creation Script
# Creates platform-specific release archives with binaries, docs, and configs

param(
    [string]$Version = "1.3.4",
    [string]$Platform = "windows-x64",
    [string]$BuildDir = "build-msvc\Release",
    [string]$OutputDir = "release",
    [switch]$IncludeBenchmarks = $false,
    [switch]$CreateChecksums = $true,
    [switch]$PrepareMiniLlm = $false,
    [string]$PythonExecutable = "python",
    [string]$MiniLlmSourceFile = ""
)

$ErrorActionPreference = "Stop"

Write-Host "╔════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  ThemisDB v$Version Release Packaging          ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

# Helper functions
function Write-Step {
    param([string]$Message)
    Write-Host "► $Message" -ForegroundColor Green
}

function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor Green
}

function Copy-WithLogging {
    param(
        [string]$Source,
        [string]$Destination,
        [string]$Description
    )
    
    if (Test-Path $Source) {
        Copy-Item -Path $Source -Destination $Destination -Force
        Write-Host "  + $Description" -ForegroundColor Gray
    } else {
        Write-Host "  - $Description (not found)" -ForegroundColor Yellow
    }
}

# Step 1: Create output directory structure
Write-Step "Creating release directory structure..."

$releaseRoot = Join-Path $OutputDir "themis-v$Version-$Platform"
$releaseBin = Join-Path $releaseRoot "bin"
$releaseDocs = Join-Path $releaseRoot "docs"
$releaseConfig = Join-Path $releaseRoot "config"
$releaseLicense = Join-Path $releaseRoot "licenses"
$releaseData = Join-Path $releaseRoot "data"
$releaseModels = Join-Path $releaseRoot "models"

@($releaseRoot, $releaseBin, $releaseDocs, $releaseConfig, $releaseLicense, $releaseData, $releaseModels) | ForEach-Object {
    if (Test-Path $_) {
        Remove-Item -Path $_ -Recurse -Force
    }
    New-Item -ItemType Directory -Path $_ -Force | Out-Null
}

Write-Success "Directory structure created"

# Step 2: Copy binaries
Write-Step "Copying binaries..."

Copy-WithLogging `
    -Source "$BuildDir\themis_server.exe" `
    -Destination "$releaseBin\themis_server.exe" `
    -Description "themis_server.exe"

Copy-WithLogging `
    -Source "$BuildDir\themis_cli.exe" `
    -Destination "$releaseBin\themis_cli.exe" `
    -Description "themis_cli.exe"

Copy-WithLogging `
    -Source "$BuildDir\themis_demo.exe" `
    -Destination "$releaseBin\themis_demo.exe" `
    -Description "themis_demo.exe"

# Copy DLLs if they exist
$dlls = Get-ChildItem -Path $BuildDir -Filter "*.dll" -ErrorAction SilentlyContinue
if ($dlls) {
    Write-Host "  Copying DLL dependencies..." -ForegroundColor Gray
    $dlls | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination $releaseBin -Force
        Write-Host "    + $($_.Name)" -ForegroundColor DarkGray
    }
}

Write-Success "Binaries copied"

# Step 3: Copy benchmarks (optional)
if ($IncludeBenchmarks) {
    Write-Step "Copying benchmarks..."
    $benchmarkDir = Join-Path $releaseBin "benchmarks"
    New-Item -ItemType Directory -Path $benchmarkDir -Force | Out-Null
    
    $benchmarks = @(
        "bench_v1_3_4_optimizations.exe",
        "bench_batch_insert.exe",
        "themis_benchmarks.exe"
    )
    
    foreach ($bench in $benchmarks) {
        Copy-WithLogging `
            -Source "$BuildDir\$bench" `
            -Destination "$benchmarkDir\$bench" `
            -Description $bench
    }
    
    Write-Success "Benchmarks copied"
}

# Step 4: Copy documentation
Write-Step "Copying documentation..."

$docFiles = @(
    @{Source = "README.md"; Dest = "README.md"},
    @{Source = "CHANGELOG.md"; Dest = "CHANGELOG.md"},
    @{Source = "RELEASE_NOTES_v$Version.md"; Dest = "RELEASE_NOTES.md"},
    @{Source = "BATCH_INSERT_PERFORMANCE_RESULTS.md"; Dest = "BATCH_INSERT_PERFORMANCE.md"},
    @{Source = "LICENSE"; Dest = "LICENSE"},
    @{Source = "CONTRIBUTING.md"; Dest = "CONTRIBUTING.md"}
)

foreach ($doc in $docFiles) {
    Copy-WithLogging `
        -Source $doc.Source `
        -Destination (Join-Path $releaseDocs $doc.Dest) `
        -Description $doc.Dest
}

# Copy additional docs
if (Test-Path "docs\README.md") {
    Copy-Item -Path "docs\*" -Destination $releaseDocs -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Success "Documentation copied"

# Step 5: Copy configuration files
Write-Step "Copying configuration files..."

$configFiles = @(
    "config\themis.yaml.example",
    "config\logging.yaml"
)

foreach ($config in $configFiles) {
    if (Test-Path $config) {
        Copy-WithLogging `
            -Source $config `
            -Destination (Join-Path $releaseConfig (Split-Path $config -Leaf)) `
            -Description (Split-Path $config -Leaf)
    }
}

Write-Success "Configuration files copied"

# Step 5b: Copy docs database and optional mini LLM bundle
Write-Step "Copying documentation database and LLM assets..."

$docsDbCandidates = @(
    (Join-Path (Split-Path $BuildDir -Parent) "data\docs.db"),
    "data\docs.db"
)

$docsJsonCandidates = @(
    (Join-Path (Split-Path $BuildDir -Parent) "data\docs_database.json"),
    "data\docs_database.json"
)

foreach ($candidate in $docsDbCandidates | Select-Object -Unique) {
    if (Test-Path $candidate) {
        Copy-WithLogging -Source $candidate -Destination (Join-Path $releaseData "docs.db") -Description "docs.db"
        break
    }
}

foreach ($candidate in $docsJsonCandidates | Select-Object -Unique) {
    if (Test-Path $candidate) {
        Copy-WithLogging -Source $candidate -Destination (Join-Path $releaseData "docs_database.json") -Description "docs_database.json"
        break
    }
}

if ($PrepareMiniLlm) {
    $miniLlmScript = Join-Path (Get-Location) "scripts\prepare_release_mini_llm.py"
    if (Test-Path $miniLlmScript) {
        $helperArgs = @($miniLlmScript, "--output-dir", $releaseModels)
        if ($MiniLlmSourceFile) {
            $helperArgs += @("--source-file", $MiniLlmSourceFile)
        }

        try {
            & $PythonExecutable @helperArgs
            if ($LASTEXITCODE -eq 0) {
                Write-Success "Mini LLM bundle prepared"
            } else {
                Write-Host "  - Mini LLM bundle preparation failed" -ForegroundColor Yellow
            }
        } catch {
            Write-Host "  - Mini LLM bundle preparation failed: $_" -ForegroundColor Yellow
        }
    } else {
        Write-Host "  - Mini LLM helper script not found" -ForegroundColor Yellow
    }
} else {
    $modelCandidates = @(
        "models\default.gguf",
        "models\mini-llm.manifest.json"
    )

    foreach ($candidate in $modelCandidates) {
        if (Test-Path $candidate) {
            Copy-WithLogging -Source $candidate -Destination (Join-Path $releaseModels (Split-Path $candidate -Leaf)) -Description (Split-Path $candidate -Leaf)
        }
    }
}

Write-Success "Documentation database and LLM assets handled"

# Step 6: Create startup scripts
Write-Step "Creating startup scripts..."

# Windows batch script
$startBat = @"
@echo off
echo ╔════════════════════════════════════════════════╗
echo ║  ThemisDB v$Version - Starting Server...        ║
echo ╚════════════════════════════════════════════════╝
echo.

cd /d "%~dp0"

if not exist "data" (
    echo Creating data directory...
    mkdir data
)

echo Starting ThemisDB server...
bin\themis_server.exe --config config\themis.yaml.example %*

if errorlevel 1 (
    echo.
    echo Server exited with error code %errorlevel%
    pause
)
"@

$startBat | Out-File -FilePath (Join-Path $releaseRoot "start-server.bat") -Encoding ASCII

# PowerShell script
$startPs1 = @"
# ThemisDB Server Startup Script
param(
    [string]`$Config = "config\themis.yaml.example",
    [string[]]`$AdditionalArgs = @()
)

Write-Host "╔════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  ThemisDB v$Version - Starting Server...        ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

`$scriptDir = Split-Path -Parent `$MyInvocation.MyCommand.Path
Set-Location `$scriptDir

if (-not (Test-Path "data")) {
    Write-Host "Creating data directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path "data" | Out-Null
}

Write-Host "Starting ThemisDB server..." -ForegroundColor Green
Write-Host "Config: `$Config" -ForegroundColor Gray
Write-Host ""

& "bin\themis_server.exe" --config `$Config `$AdditionalArgs

if (`$LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "Server exited with error code `$LASTEXITCODE" -ForegroundColor Red
}
"@

$startPs1 | Out-File -FilePath (Join-Path $releaseRoot "start-server.ps1") -Encoding UTF8

Write-Success "Startup scripts created"

# Step 7: Create README for release
Write-Step "Creating release README..."

$releaseReadme = @"
# ThemisDB v$Version - $Platform

## Quick Start

### Windows

**Start the server:**
``````batch
start-server.bat
``````

Or with PowerShell:
``````powershell
.\start-server.ps1
``````

**Connect to server:**
- HTTP API: http://localhost:8080
- Bolt Protocol: bolt://localhost:7687

### Configuration

Edit `config\themis.yaml.example` to customize settings.

### Documentation

See `docs\` folder for complete documentation:
- Release notes: `docs\RELEASE_NOTES.md`
- Performance results: `docs\BATCH_INSERT_PERFORMANCE.md`
- Full documentation: https://github.com/makr-code/ThemisDB

## What's New in v$Version

### 🚀 Major Performance Improvements

- **23-77x faster** bulk inserts via new Batch Insert API
- **98.2% latency reduction** for 100-entity batches
- **60-200x faster** index metadata lookups

### 🆕 New Features

- Batch Insert API for optimal bulk operations
- Automatic metadata caching
- Enhanced benchmarking suite

See `docs\RELEASE_NOTES.md` for full details.

## Directory Structure

``````
themis-v$Version-$Platform/
├── bin/                    # Executables and DLLs
│   ├── themis_server.exe  # Main server
│   ├── themis_cli.exe     # Command-line client
│   └── themis_demo.exe    # Demo application
├── data/                   # Prebuilt docs database assets
├── models/                 # Optional llama.cpp GGUF model bundle
├── config/                 # Configuration files
├── docs/                   # Documentation
├── licenses/               # License information
├── start-server.bat       # Windows batch starter
└── start-server.ps1       # PowerShell starter
``````

## System Requirements

- Windows 10/11 or Windows Server 2019+
- 4 GB RAM minimum (8 GB recommended)
- 1 GB disk space for installation
- Visual C++ Redistributable 2022

## Support

- GitHub: https://github.com/makr-code/ThemisDB
- Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://themisdb.io/docs

## License

See `LICENSE` file for details.
"@

$releaseReadme | Out-File -FilePath (Join-Path $releaseRoot "README.txt") -Encoding UTF8

Write-Success "Release README created"

# Step 8: Create archives
Write-Step "Creating ZIP archive..."

$zipFile = Join-Path $OutputDir "themis-v$Version-$Platform.zip"
if (Test-Path $zipFile) {
    Remove-Item $zipFile -Force
}

# Use PowerShell's Compress-Archive
Compress-Archive -Path $releaseRoot -DestinationPath $zipFile -CompressionLevel Optimal

$zipSize = (Get-Item $zipFile).Length / 1MB
Write-Success "ZIP archive created ($([math]::Round($zipSize, 2)) MB)"

# Step 9: Create checksums
if ($CreateChecksums) {
    Write-Step "Generating checksums..."
    
    $checksumFile = Join-Path $OutputDir "themis-v$Version-$Platform.sha256"
    
    $hash = Get-FileHash -Path $zipFile -Algorithm SHA256
    $checksumContent = "$($hash.Hash.ToLower())  themis-v$Version-$Platform.zip"
    $checksumContent | Out-File -FilePath $checksumFile -Encoding ASCII
    
    Write-Success "Checksum file created"
    Write-Host "  SHA256: $($hash.Hash.ToLower())" -ForegroundColor Gray
}

# Step 10: Summary
Write-Host ""
Write-Host "╔════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║     Release Archive Created Successfully!      ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""
Write-Host "Release artifacts:" -ForegroundColor Cyan
Write-Host "  Archive: $zipFile" -ForegroundColor White
if ($CreateChecksums) {
    Write-Host "  Checksum: $checksumFile" -ForegroundColor White
}
Write-Host "  Directory: $releaseRoot" -ForegroundColor White
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "1. Upload ZIP and SHA256 to GitHub Release" -ForegroundColor White
Write-Host "2. Test the archive on a clean system" -ForegroundColor White
Write-Host "3. Update release notes with download links" -ForegroundColor White
Write-Host ""

# Return path to archive for automation
return @{
    ArchivePath = $zipFile
    ChecksumPath = $checksumFile
    Size = $zipSize
    Hash = $(if ($CreateChecksums) { $hash.Hash } else { $null })
}
