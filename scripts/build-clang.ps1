#!/usr/bin/env pwsh
# Build ThemisDB with Clang/LLVM and Ninja
param(
    [switch]$Clean,
    [switch]$Configure,
    [int]$Jobs = 8,
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

$RepoRoot = "C:\VCC\themis"
$BuildDir = "$RepoRoot\build-clang"
$LLVMPath = "C:\Program Files\LLVM\bin"

# Check if Clang exists
if (-not (Test-Path "$LLVMPath\clang.exe")) {
    Write-Host "ERROR: Clang not found at $LLVMPath" -ForegroundColor Red
    Write-Host "Please install LLVM from https://github.com/llvm/llvm-project/releases" -ForegroundColor Yellow
    exit 1
}

# Display Clang version
Write-Host "=== Clang Compiler Information ===" -ForegroundColor Cyan
& "$LLVMPath\clang.exe" --version
Write-Host ""

# Clean if requested
if ($Clean) {
    Write-Host "=== Cleaning build directory ===" -ForegroundColor Cyan
    Remove-Item $BuildDir -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

# Add LLVM to PATH
$env:PATH = "$LLVMPath;$env:PATH"

# Configure with CMake if requested
if ($Configure -or -not (Test-Path "$BuildDir\build.ninja")) {
    Write-Host "=== Configuring CMake with Clang and Ninja ===" -ForegroundColor Cyan
    Push-Location $RepoRoot
    
    $env:VCPKG_ROOT = "$RepoRoot\vcpkg"
    $env:CC = "$LLVMPath\clang.exe"
    $env:CXX = "$LLVMPath\clang++.exe"
    
    cmake -S . -B $BuildDir `
        -G Ninja `
        "-DCMAKE_BUILD_TYPE=$BuildType" `
        "-DCMAKE_C_COMPILER=$LLVMPath\clang.exe" `
        "-DCMAKE_CXX_COMPILER=$LLVMPath\clang++.exe" `
        "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
        -DTHEMIS_BUILD_TESTS=OFF `
        -DTHEMIS_BUILD_BENCHMARKS=OFF `
        -DTHEMIS_BUILD_DOCS_DB=OFF `
        -DTHEMIS_ENABLE_LLM=OFF
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "✗ CMake configuration failed" -ForegroundColor Red
        Pop-Location
        exit $LASTEXITCODE
    }
    
    Write-Host "✓ CMake configured successfully with Clang" -ForegroundColor Green
    Pop-Location
}

# Build with Ninja
Write-Host "=== Building with Ninja (parallel jobs: $Jobs) ===" -ForegroundColor Cyan
Push-Location $BuildDir

ninja -j $Jobs

$buildResult = $LASTEXITCODE

if ($buildResult -eq 0) {
    Write-Host "✓ Build completed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Build artifacts:" -ForegroundColor Cyan
    Get-ChildItem . -Recurse -Include "*.exe", "*.dll", "*.lib" | Select-Object Name, Length, Directory | Format-Table -AutoSize
} else {
    Write-Host "✗ Build failed (Exit Code: $buildResult)" -ForegroundColor Red
}

Pop-Location
exit $buildResult
