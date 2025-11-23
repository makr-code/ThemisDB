# Unified Build Script for ThemisDB
# Supports: Windows (MSVC/ClangCL), Linux/WSL (GCC/Clang), Docker, ARM64
#
# Usage:
#   .\build-unified.ps1 -Platform windows -Config release
#   .\build-unified.ps1 -Platform docker -Tag themisdb:latest
#   .\build-unified.ps1 -Platform qnap -Static
#   .\build-unified.ps1 -Clean

param(
    [ValidateSet('windows', 'linux', 'wsl', 'docker', 'qnap', 'arm64', 'rpi')]
    [string]$Platform = 'windows',
    
    [ValidateSet('debug', 'release')]
    [string]$Config = 'release',
    
    [ValidateSet('msvc', 'clang', 'clangcl', 'gcc')]
    [string]$Compiler = 'msvc',
    
    [string]$Tag = 'themisdb:latest',
    
    [switch]$Static,
    [switch]$Clean,
    [switch]$Tests,
    [switch]$Benchmarks,
    [switch]$NoCache
)

$ErrorActionPreference = "Stop"

# ============================================================================
# Configuration
# ============================================================================

$Version = "0.1.0"  # TODO: Read from CMakeLists.txt
$ProjectRoot = $PSScriptRoot

Write-Host "=== ThemisDB Unified Build System ===" -ForegroundColor Cyan
Write-Host "Platform: $Platform | Config: $Config | Compiler: $Compiler" -ForegroundColor Gray
Write-Host ""

# ============================================================================
# Platform Detection & Validation
# ============================================================================

function Get-CMakePreset {
    param($Platform, $Compiler, $Config)
    
    $presetMap = @{
        'windows-msvc-debug' = 'windows-ninja-msvc-debug'
        'windows-msvc-release' = 'windows-ninja-msvc-release'
        'windows-clangcl-debug' = 'windows-ninja-clangcl-debug'
        'windows-clangcl-release' = 'windows-ninja-clangcl-release'
        'linux-clang-debug' = 'linux-ninja-clang-debug'
        'linux-clang-release' = 'linux-ninja-clang-release'
        'linux-gcc-debug' = 'linux-ninja-gcc-debug'
        'linux-gcc-release' = 'linux-ninja-gcc-release'
        'arm64-gcc-debug' = 'linux-arm64-gcc-debug'
        'arm64-gcc-release' = 'linux-arm64-gcc-release'
        'rpi-gcc-debug' = 'rpi-arm64-gcc-debug'
        'rpi-gcc-release' = 'rpi-arm64-gcc-release'
    }
    
    $key = "$Platform-$Compiler-$Config"
    if ($presetMap.ContainsKey($key)) {
        return $presetMap[$key]
    }
    
    # Fallback
    return "windows-ninja-msvc-release"
}

# ============================================================================
# Clean Function
# ============================================================================

function Invoke-Clean {
    Write-Host "Cleaning build artifacts..." -ForegroundColor Yellow
    
    $buildDirs = @(
        "build-msvc-ninja-debug",
        "build-msvc-ninja-release",
        "build-clangcl-debug",
        "build-clangcl-release",
        "build-linux-clang-debug",
        "build-linux-clang-release",
        "build-arm64-debug",
        "build-arm64-release",
        "build-rpi-arm64-debug",
        "build-rpi-arm64-release",
        "build"
    )
    
    foreach ($dir in $buildDirs) {
        $path = Join-Path $ProjectRoot $dir
        if (Test-Path $path) {
            Write-Host "  Removing $dir..." -ForegroundColor Gray
            Remove-Item -Recurse -Force $path
        }
    }
    
    Write-Host "Clean complete!" -ForegroundColor Green
}

# ============================================================================
# Native Build (Windows/Linux/WSL)
# ============================================================================

function Invoke-NativeBuild {
    param($Preset, $Config, $Tests, $Benchmarks, $Static)
    
    Write-Host "Building with CMake Preset: $Preset" -ForegroundColor Yellow
    
    # Configure
    $configureArgs = @("--preset", $Preset)
    
    if ($Static) {
        $configureArgs += "-DTHEMIS_STATIC_BUILD=ON"
    }
    
    if ($Tests) {
        $configureArgs += "-DTHEMIS_BUILD_TESTS=ON"
    } else {
        $configureArgs += "-DTHEMIS_BUILD_TESTS=OFF"
    }
    
    if ($Benchmarks) {
        $configureArgs += "-DTHEMIS_BUILD_BENCHMARKS=ON"
    } else {
        $configureArgs += "-DTHEMIS_BUILD_BENCHMARKS=OFF"
    }
    
    Write-Host "Configuring..." -ForegroundColor Gray
    & cmake $configureArgs
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Configuration failed!" -ForegroundColor Red
        exit 1
    }
    
    # Build
    Write-Host "Building..." -ForegroundColor Gray
    & cmake --build --preset $Preset
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host ""
    Write-Host "=== Build Successful ===" -ForegroundColor Green
    Write-Host "Binary location: Check build directory for themis_server" -ForegroundColor Cyan
}

# ============================================================================
# Docker Build
# ============================================================================

function Invoke-DockerBuild {
    param($Tag, $Static, $NoCache)
    
    Write-Host "Building Docker image: $Tag" -ForegroundColor Yellow
    
    # Use simplified build (pre-built binary)
    if (Test-Path ".\build-docker-simple.ps1") {
        Write-Host "Using simplified Docker build (pre-built binary)..." -ForegroundColor Gray
        
        $args = @()
        if ($NoCache) { $args += "-NoCache" }
        $args += "-Tag", $Tag
        
        & ".\build-docker-simple.ps1" @args
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Docker build failed!" -ForegroundColor Red
            exit 1
        }
    } else {
        # Fallback to full Docker build
        Write-Host "Using full Docker build..." -ForegroundColor Gray
        
        $dockerArgs = @("build", "-f", "Dockerfile", "-t", $Tag)
        if ($NoCache) { $dockerArgs += "--no-cache" }
        $dockerArgs += "."
        
        & docker $dockerArgs
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Docker build failed!" -ForegroundColor Red
            exit 1
        }
    }
    
    Write-Host ""
    Write-Host "=== Docker Build Successful ===" -ForegroundColor Green
    Write-Host "Image: $Tag" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Test: docker run --rm $Tag --version" -ForegroundColor Gray
}

# ============================================================================
# QNAP Build (Special handling for Ubuntu 20.04 compatibility)
# ============================================================================

function Invoke-QNAPBuild {
    param($Tag, $Static)
    
    Write-Host "Building QNAP-compatible image (Ubuntu 20.04)..." -ForegroundColor Yellow
    
    if (-not $Static) {
        Write-Host ""
        Write-Host "WARNING: QNAP build requires static linking!" -ForegroundColor Red
        Write-Host "Re-running with -Static flag..." -ForegroundColor Yellow
        Write-Host ""
        $Static = $true
    }
    
    # TODO: Implement static build in Docker container
    Write-Host "QNAP build not yet implemented!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Workaround:" -ForegroundColor Yellow
    Write-Host "  1. Build with static linking: .\build-unified.ps1 -Platform linux -Static" -ForegroundColor Gray
    Write-Host "  2. Use build-docker-qnap-simple.ps1 (if WSL is Ubuntu 20.04)" -ForegroundColor Gray
    Write-Host ""
    exit 1
}

# ============================================================================
# Main Execution
# ============================================================================

if ($Clean) {
    Invoke-Clean
    exit 0
}

switch ($Platform) {
    'windows' {
        if ($Compiler -eq 'gcc') {
            Write-Host "GCC not supported on Windows, using MSVC" -ForegroundColor Yellow
            $Compiler = 'msvc'
        }
        
        $preset = Get-CMakePreset -Platform 'windows' -Compiler $Compiler -Config $Config
        Invoke-NativeBuild -Preset $preset -Config $Config -Tests $Tests -Benchmarks $Benchmarks -Static $Static
    }
    
    { $_ -in 'linux', 'wsl' } {
        if ($Compiler -eq 'msvc' -or $Compiler -eq 'clangcl') {
            Write-Host "MSVC/ClangCL not supported on Linux, using Clang" -ForegroundColor Yellow
            $Compiler = 'clang'
        }
        
        $preset = Get-CMakePreset -Platform 'linux' -Compiler $Compiler -Config $Config
        
        # Check if running on Windows (need WSL wrapper)
        if ($IsWindows -or $env:OS -eq 'Windows_NT') {
            Write-Host "Detected Windows, using WSL..." -ForegroundColor Gray
            $cmakeCmd = "wsl bash -lc 'cd /mnt/c/VCC/themis && cmake --preset $preset && cmake --build --preset $preset'"
            Invoke-Expression $cmakeCmd
        } else {
            Invoke-NativeBuild -Preset $preset -Config $Config -Tests $Tests -Benchmarks $Benchmarks -Static $Static
        }
    }
    
    'docker' {
        Invoke-DockerBuild -Tag $Tag -Static $Static -NoCache $NoCache
    }
    
    'qnap' {
        Invoke-QNAPBuild -Tag $Tag -Static $Static
    }
    
    { $_ -in 'arm64', 'rpi' } {
        $preset = Get-CMakePreset -Platform $Platform -Compiler 'gcc' -Config $Config
        Invoke-NativeBuild -Preset $preset -Config $Config -Tests $Tests -Benchmarks $Benchmarks -Static $Static
    }
    
    default {
        Write-Host "Unknown platform: $Platform" -ForegroundColor Red
        exit 1
    }
}

Write-Host ""
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "Version: $Version" -ForegroundColor Gray
