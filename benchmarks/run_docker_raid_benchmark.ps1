#!/usr/bin/env pwsh
# ThemisDB Docker RAID Benchmark - Quick Start Script
# Runs comprehensive 1+ hour benchmark suite with default configuration

param(
    [int]$MinTime = 60,          # Minimum seconds per benchmark
    [int]$Repetitions = 3,       # Number of repetitions for statistics
    [string]$Filter = ".*",      # Benchmark filter regex
    [string]$OutputDir = "raid_benchmark_results",
    [switch]$QuickTest = $false, # Quick smoke test (5-10 min)
    [switch]$Help = $false
)

if ($Help) {
    Write-Host @"
ThemisDB Docker RAID Benchmark Suite - Quick Start

Usage:
  .\run_docker_raid_benchmark.ps1 [options]

Options:
  -MinTime <seconds>        Minimum time per benchmark (default: 60)
  -Repetitions <n>          Number of repetitions (default: 3)
  -Filter <regex>           Benchmark filter (default: all)
  -OutputDir <path>         Output directory (default: raid_benchmark_results)
  -QuickTest               Run quick smoke test (~10 minutes)
  -Help                    Show this help

Examples:
  # Standard 1+ hour run
  .\run_docker_raid_benchmark.ps1

  # Quick smoke test
  .\run_docker_raid_benchmark.ps1 -QuickTest

  # Extended 2+ hour run with more repetitions
  .\run_docker_raid_benchmark.ps1 -MinTime 120 -Repetitions 5

  # Only RAID1 tests
  .\run_docker_raid_benchmark.ps1 -Filter ".*RAID1.*"

  # Only failover tests
  .\run_docker_raid_benchmark.ps1 -Filter ".*Failover.*"
"@
    exit 0
}

$ErrorActionPreference = "Stop"

# Colors
$ColorHeader = "Cyan"
$ColorSuccess = "Green"
$ColorWarning = "Yellow"
$ColorError = "Red"

function Write-Header {
    param([string]$Message)
    Write-Host "`n========================================================================" -ForegroundColor $ColorHeader
    Write-Host $Message -ForegroundColor $ColorHeader
    Write-Host "========================================================================`n" -ForegroundColor $ColorHeader
}

function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor $ColorSuccess
}

function Write-Warning {
    param([string]$Message)
    Write-Host "⚠ $Message" -ForegroundColor $ColorWarning
}

function Write-ErrorMsg {
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor $ColorError
}

Write-Header "ThemisDB Docker RAID Comprehensive Benchmark Suite"

# Configuration
$BenchmarkExe = ""
$BuildDir = "build-msvc"
$SourceFile = "compendium\benchmarks\bench_docker_raid_comprehensive.cpp"

# Adjust for quick test
if ($QuickTest) {
    Write-Warning "Quick test mode enabled - runtime ~10 minutes"
    $MinTime = 2
    $Repetitions = 1
}

# Find benchmark executable
Write-Host "Searching for benchmark executable..."
$PossiblePaths = @(
    "$BuildDir\Release\bench_docker_raid_comprehensive.exe",
    "$BuildDir\Debug\bench_docker_raid_comprehensive.exe",
    "bench_docker_raid_comprehensive.exe",
    ".\bench_docker_raid_comprehensive.exe"
)

foreach ($Path in $PossiblePaths) {
    if (Test-Path $Path) {
        $BenchmarkExe = $Path
        Write-Success "Found: $BenchmarkExe"
        break
    }
}

if (-not $BenchmarkExe) {
    Write-ErrorMsg "Benchmark executable not found!"
    Write-Host "`nTrying to build..."
    
    if (-not (Test-Path $BuildDir)) {
        Write-Host "Build directory not found. Running CMake configuration..."
        
        cmake -S . -B $BuildDir -G "Visual Studio 17 2022" -A x64 `
            -DCMAKE_TOOLCHAIN_FILE="vcpkg\scripts\buildsystems\vcpkg.cmake" `
            -DVCPKG_TARGET_TRIPLET=x64-windows
        
        if ($LASTEXITCODE -ne 0) {
            Write-ErrorMsg "CMake configuration failed"
            exit 1
        }
    }
    
    Write-Host "Building benchmark..."
    cmake --build $BuildDir --target bench_docker_raid_comprehensive --config Release -j 8
    
    if ($LASTEXITCODE -ne 0) {
        Write-ErrorMsg "Build failed"
        exit 1
    }
    
    $BenchmarkExe = "$BuildDir\Release\bench_docker_raid_comprehensive.exe"
    
    if (-not (Test-Path $BenchmarkExe)) {
        Write-ErrorMsg "Build succeeded but executable not found at: $BenchmarkExe"
        exit 1
    }
    
    Write-Success "Build completed successfully"
}

# Check Docker
Write-Host "`nChecking Docker..."
try {
    docker ps | Out-Null
    Write-Success "Docker is running"
} catch {
    Write-ErrorMsg "Docker is not running or not accessible"
    Write-Host "Please start Docker Desktop and try again"
    exit 1
}

# Create output directory
Write-Host "`nPreparing output directory..."
$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$FullOutputDir = Join-Path $OutputDir "run_$Timestamp"
New-Item -ItemType Directory -Path $FullOutputDir -Force | Out-Null
Write-Success "Output directory: $FullOutputDir"

# Prepare arguments
$BenchmarkArgs = @(
    "--benchmark_min_time=$MinTime",
    "--benchmark_repetitions=$Repetitions",
    "--benchmark_report_aggregates_only=true",
    "--benchmark_out=$FullOutputDir\results.json",
    "--benchmark_out_format=json",
    "--benchmark_filter=$Filter"
)

# Display configuration
Write-Header "Benchmark Configuration"
Write-Host "Executable:      $BenchmarkExe"
Write-Host "Min Time:        $MinTime seconds per benchmark"
Write-Host "Repetitions:     $Repetitions"
Write-Host "Filter:          $Filter"
Write-Host "Output Dir:      $FullOutputDir"
Write-Host ""

if ($QuickTest) {
    Write-Host "Mode:            Quick Test (~10 minutes)" -ForegroundColor $ColorWarning
    Write-Host "Estimated Time:  ~10-15 minutes" -ForegroundColor $ColorWarning
} else {
    Write-Host "Mode:            Full Suite" -ForegroundColor $ColorSuccess
    Write-Host "Estimated Time:  ~1-2 hours" -ForegroundColor $ColorSuccess
}

Write-Host ""
Write-Host "Press Ctrl+C to abort, or wait 5 seconds to start..." -ForegroundColor $ColorWarning
Start-Sleep -Seconds 5

# Run benchmark
Write-Header "Running Benchmark Suite"
$StartTime = Get-Date

try {
    $Process = Start-Process -FilePath $BenchmarkExe `
        -ArgumentList $BenchmarkArgs `
        -NoNewWindow `
        -PassThru `
        -Wait
    
    $ExitCode = $Process.ExitCode
    
    $EndTime = Get-Date
    $Duration = $EndTime - $StartTime
    
    if ($ExitCode -eq 0) {
        Write-Header "Benchmark Completed Successfully"
        Write-Success "Total runtime: $($Duration.ToString('hh\:mm\:ss'))"
        
        # Display results location
        Write-Host "`nResults saved to:"
        Write-Host "  JSON:  $FullOutputDir\results.json" -ForegroundColor $ColorSuccess
        
        # Check if results exist
        if (Test-Path "$FullOutputDir\results.json") {
            $ResultSize = (Get-Item "$FullOutputDir\results.json").Length
            Write-Host "  Size:  $([math]::Round($ResultSize / 1KB, 2)) KB"
            
            # Parse some basic stats from JSON
            try {
                $Results = Get-Content "$FullOutputDir\results.json" | ConvertFrom-Json
                $NumBenchmarks = $Results.benchmarks.Count
                Write-Host "  Tests: $NumBenchmarks benchmark runs"
            } catch {
                Write-Warning "Could not parse JSON results"
            }
        }
        
        # Save run info
        $RunInfo = @{
            Timestamp = $Timestamp
            StartTime = $StartTime.ToString("yyyy-MM-dd HH:mm:ss")
            EndTime = $EndTime.ToString("yyyy-MM-dd HH:mm:ss")
            Duration = $Duration.ToString("hh\:mm\:ss")
            MinTime = $MinTime
            Repetitions = $Repetitions
            Filter = $Filter
            QuickTest = $QuickTest
            ExitCode = $ExitCode
        }
        
        $RunInfo | ConvertTo-Json | Out-File "$FullOutputDir\run_info.json"
        
        Write-Host "`nNext steps:"
        Write-Host "  1. Analyze results: notepad $FullOutputDir\results.json"
        Write-Host "  2. Compare runs:    python compare_benchmark_results.py"
        Write-Host "  3. Generate report: python analyze_raid_benchmarks.py $FullOutputDir"
        
    } else {
        Write-ErrorMsg "Benchmark failed with exit code: $ExitCode"
        exit $ExitCode
    }
    
} catch {
    Write-ErrorMsg "Error running benchmark: $_"
    exit 1
}

Write-Host ""
