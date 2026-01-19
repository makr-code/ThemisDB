#!/usr/bin/env pwsh
# Initialize MSVC environment and build
$ErrorActionPreference = 'Stop'

$vsDevCmd = 'C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat'
$projectRoot = 'C:\VCC\themis'

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Initializing MSVC Environment"
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Initialize MSVC environment via cmd
$initCmd = @"
"$vsDevCmd" -arch=x64 && cd /d "$projectRoot" && set
"@

$envVars = @{}
& cmd /c $initCmd | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') {
        $envVars[$Matches[1]] = $Matches[2]
    }
}

# Set environment variables
foreach ($key in $envVars.Keys) {
    [Environment]::SetEnvironmentVariable($key, $envVars[$key], 'Process')
}

Write-Host "✓ MSVC environment initialized" -ForegroundColor Green
Write-Host ""

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Configuring CMake"
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

Push-Location $projectRoot

# Remove old build dir
if (Test-Path build-ninja) {
    Write-Host "Removing old build-ninja directory..."
    Remove-Item -Recurse -Force build-ninja
}

mkdir build-ninja | Out-Null

# Configure CMake
$cmakeBuild = cmake -S . -B build-ninja `
    -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="$projectRoot\vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=x64-windows-static `
    -DTHEMIS_BUILD_TESTS=ON `
    -DTHEMIS_ENABLE_LLM=ON `
    -DTHEMIS_BUILD_BENCHMARKS=ON `
    2>&1

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ CMake configuration completed successfully" -ForegroundColor Green
} else {
    Write-Host "✗ CMake configuration failed" -ForegroundColor Red
    $cmakeBuild | Select-Object -Last 30
    Pop-Location
    exit 1
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Building themis_server"
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Build themis_server
$buildOutput = ninja -C build-ninja themis_server -j 4 2>&1

$buildOutput | Select-String 'error|lora|Compiling|Built' | Select-Object -First 50

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Build completed successfully" -ForegroundColor Green
} else {
    Write-Host "✗ Build failed - see errors above" -ForegroundColor Red
    Pop-Location
    exit 1
}

Pop-Location
Write-Host ""
Write-Host "All done!" -ForegroundColor Green
