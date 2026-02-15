# Build ThemisDB with Ninja (requires Visual Studio 2022)
param(
    [switch]$Clean,
    [switch]$Configure,
    [int]$Jobs = 8
)

$ErrorActionPreference = "Stop"

$RepoRoot = "C:\VCC\themis"
$BuildDir = "$RepoRoot\build-msvc-ninja-release"
$VcVarsAll = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"

# Check if vcvarsall.bat exists
if (-not (Test-Path $VcVarsAll)) {
    Write-Host "ERROR: vcvarsall.bat not found at $VcVarsAll" -ForegroundColor Red
    Write-Host "Please adjust the path to match your Visual Studio installation" -ForegroundColor Yellow
    exit 1
}

# Clean if requested
if ($Clean) {
    Write-Host "=== Cleaning build directory ===" -ForegroundColor Cyan
    Remove-Item $BuildDir -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

# Load Visual Studio environment
Write-Host "=== Loading Visual Studio 2022 Environment ===" -ForegroundColor Cyan
$env:VSCMD_ARG_TGT_ARCH = "x64"
$env:VSCMD_ARG_HOST_ARCH = "x64"

# Import VS environment variables using cmd
cmd /c "`"$VcVarsAll`" x64 && set" | ForEach-Object {
    if ($_ -match "^(.*?)=(.*)$") {
        Set-Content "env:\$($matches[1])" $matches[2]
    }
}

Write-Host "✓ Visual Studio environment loaded" -ForegroundColor Green

# Configure with CMake if requested
if ($Configure -or -not (Test-Path "$BuildDir\build.ninja")) {
    Write-Host "=== Configuring CMake with Ninja ===" -ForegroundColor Cyan
    Push-Location $RepoRoot
    
    $env:VCPKG_ROOT = "$RepoRoot\vcpkg"
    
    cmake -S . -B $BuildDir `
        -G Ninja `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
        -DTHEMIS_BUILD_TESTS=ON `
        -DTHEMIS_BUILD_BENCHMARKS=ON `
        -DTHEMIS_BUILD_DOCS_DB=OFF `
        -DTHEMIS_ENABLE_LLM=OFF
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "✗ CMake configuration failed" -ForegroundColor Red
        Pop-Location
        exit $LASTEXITCODE
    }
    
    Write-Host "✓ CMake configured successfully" -ForegroundColor Green
    Pop-Location
}

# Build with Ninja
Write-Host "=== Building with Ninja (parallel jobs: $Jobs) ===" -ForegroundColor Cyan
Push-Location $BuildDir

ninja -j $Jobs

$buildResult = $LASTEXITCODE

if ($buildResult -eq 0) {
    Write-Host "✓ Build completed successfully!" -ForegroundColor Green
} else {
    Write-Host "✗ Build failed (Exit Code: $buildResult)" -ForegroundColor Red
}

Pop-Location
exit $buildResult
