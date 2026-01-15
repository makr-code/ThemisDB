# Build ThemisDB with Ninja (Windows mit Tests und Benchmarks)
param(
    [switch]$Clean,
    [int]$Jobs = 8
)

$ErrorActionPreference = "Stop"

$RepoRoot = "C:\VCC\themis"
$BuildDir = "$RepoRoot\build-ninja"
$VcVarsAll = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"

Write-Host "=== Windows Ninja Build mit Tests + Benchmarks ===" -ForegroundColor Cyan

# Check if vcvarsall.bat exists
if (-not (Test-Path $VcVarsAll)) {
    Write-Host "ERROR: vcvarsall.bat not found at $VcVarsAll" -ForegroundColor Red
    exit 1
}

# Clean if requested
if ($Clean) {
    Write-Host "[1/4] Cleaning build directory..." -ForegroundColor Green
    Remove-Item $BuildDir -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

# Load Visual Studio environment
Write-Host "[2/4] Loading Visual Studio 2022 Environment..." -ForegroundColor Green
$env:VSCMD_ARG_TGT_ARCH = "x64"
$env:VSCMD_ARG_HOST_ARCH = "x64"

cmd /c "`"$VcVarsAll`" x64 && set" | ForEach-Object {
    if ($_ -match "^(.*?)=(.*)$") {
        Set-Content "env:\$($matches[1])" $matches[2]
    }
}

Write-Host "✓ Visual Studio environment loaded" -ForegroundColor Green

# Configure with CMake
Write-Host "[3/4] Configuring CMake with Ninja (Tests=ON, Benchmarks=ON)..." -ForegroundColor Green
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
    exit 1
}

Write-Host "✓ CMake configured successfully" -ForegroundColor Green
Pop-Location

# Build with Ninja
Write-Host "[4/4] Building with Ninja (parallel jobs: $Jobs)..." -ForegroundColor Green
Push-Location $BuildDir

ninja -j $Jobs

$buildResult = $LASTEXITCODE

Pop-Location

if ($buildResult -eq 0) {
    Write-Host "✓ Build completed successfully!" -ForegroundColor Green
    Write-Host "Binary: $BuildDir\cmake\themis_server.exe" -ForegroundColor Green
    Write-Host "Tests available in: $BuildDir\tests\" -ForegroundColor Green
    Write-Host "Benchmarks available in: $BuildDir\benchmarks\" -ForegroundColor Green
} else {
    Write-Host "✗ Build failed (Exit Code: $buildResult)" -ForegroundColor Red
}

exit $buildResult
