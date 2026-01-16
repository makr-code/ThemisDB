#!/usr/bin/env pwsh
# Build Tests and Benchmarks for ThemisDB with proper MSVC environment

$ErrorActionPreference = 'Stop'

Write-Host "[1/4] Setting up MSVC environment..." -ForegroundColor Green

# VsDevCmd.bat path
$vsDevCmd = 'C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat'

if (-not (Test-Path $vsDevCmd)) {
    Write-Host "ERROR: VsDevCmd.bat not found at: $vsDevCmd" -ForegroundColor Red
    exit 1
}

# Build directory
$buildDir = 'C:\VCC\themis\build-ninja-tests-bench'

if (-not (Test-Path $buildDir)) {
    Write-Host "ERROR: Build directory not found: $buildDir" -ForegroundColor Red
    Write-Host "Please run configuration first with cmake" -ForegroundColor Yellow
    exit 1
}

Write-Host "[2/4] Configuring with VsDevCmd + Ninja..." -ForegroundColor Green

# Use cmd.exe to execute VsDevCmd.bat and then ninja
# We need to chain commands properly in CMD
$buildCommand = @"
"$vsDevCmd" -arch=x64 >nul && cd "$buildDir" && ninja themis_core 2>&1
"@

Write-Host "Building themis_core library..." -ForegroundColor Cyan
$result = cmd.exe /c $buildCommand

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: themis_core build failed with exit code $LASTEXITCODE" -ForegroundColor Red
    Write-Host $result
    exit $LASTEXITCODE
}

Write-Host "[3/4] Building test executables..." -ForegroundColor Green

# List of test targets to build
$testTargets = @(
    'test_snapshot_manager',
    'test_pitr_manager',
    'test_schema_manager',
    'test_snapshot_integration',
    'test_diff_engine'
)

foreach ($target in $testTargets) {
    Write-Host "  Building $target..." -ForegroundColor Yellow
    $buildCommand = @"
"$vsDevCmd" -arch=x64 >nul && cd "$buildDir" && ninja $target 2>&1
"@
    
    $result = cmd.exe /c $buildCommand
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  WARNING: $target build failed (may not exist)" -ForegroundColor Yellow
    } else {
        Write-Host "  ✓ $target built successfully" -ForegroundColor Green
    }
}

Write-Host "[4/4] Building benchmark executables..." -ForegroundColor Green

# List of benchmark targets to build (sample, add more as needed)
$benchTargets = @(
    'bench_core_performance',
    'bench_compression',
    'bench_auto_buffers'
)

foreach ($target in $benchTargets) {
    Write-Host "  Building $target..." -ForegroundColor Yellow
    $buildCommand = @"
"$vsDevCmd" -arch=x64 >nul && cd "$buildDir" && ninja $target 2>&1
"@
    
    $result = cmd.exe /c $buildCommand
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  WARNING: $target build failed (may not exist)" -ForegroundColor Yellow
    } else {
        Write-Host "  ✓ $target built successfully" -ForegroundColor Green
    }
}

Write-Host ""
Write-Host "Build completed!" -ForegroundColor Green
Write-Host "Test executables are in: $buildDir\tests\" -ForegroundColor Cyan
Write-Host "Benchmark executables are in: $buildDir\benchmarks\" -ForegroundColor Cyan
Write-Host ""
Write-Host "Run tests with: ctest --test-dir $buildDir --output-on-failure" -ForegroundColor Yellow
