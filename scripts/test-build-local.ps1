# Quick CI Test Script
# Tests build configuration locally without full GitHub Actions overhead

Write-Host "=== ThemisDB Local Build Test ===" -ForegroundColor Cyan
Write-Host "Testing CMake configuration and compilation...`n" -ForegroundColor Gray

# Check prerequisites
Write-Host "Checking prerequisites..." -ForegroundColor Yellow
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "❌ CMake not found. Please install CMake 3.28+" -ForegroundColor Red
    exit 1
}
Write-Host "✅ CMake: $($cmake.Version)" -ForegroundColor Green

# Create test build directory
$testBuildDir = "build-ci-test"
if (Test-Path $testBuildDir) {
    Write-Host "`nCleaning old build directory..." -ForegroundColor Yellow
    Remove-Item $testBuildDir -Recurse -Force
}

Write-Host "`n📦 Creating minimal vcpkg stub..." -ForegroundColor Yellow
$vcpkgStubDir = "vcpkg-stub/scripts/buildsystems"
New-Item -ItemType Directory -Force -Path $vcpkgStubDir | Out-Null

$stubContent = @'
# Minimal vcpkg toolchain stub for local testing
message(STATUS "Using vcpkg stub (system libraries preferred)")
set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG FALSE)
'@
$stubContent | Out-File -FilePath "$vcpkgStubDir/vcpkg.cmake" -Encoding ASCII

Write-Host "✅ vcpkg stub created`n" -ForegroundColor Green

# Configure CMake
Write-Host "🔧 Configuring CMake..." -ForegroundColor Yellow
$configArgs = @(
    "-S", ".",
    "-B", $testBuildDir,
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_TOOLCHAIN_FILE=$PWD/vcpkg-stub/scripts/buildsystems/vcpkg.cmake",
    "-DTHEMIS_BUILD_TESTS=ON",
    "-DTHEMIS_BUILD_BENCHMARKS=OFF"
)

$configResult = & cmake @configArgs 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "⚠️  CMake configuration warnings/errors:" -ForegroundColor Yellow
    $configResult | Select-String -Pattern "(ERROR|WARNING|Could NOT find)" | 
        ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
} else {
    Write-Host "✅ CMake configuration successful" -ForegroundColor Green
}

# Try to build core library
Write-Host "`n🔨 Building themis_core..." -ForegroundColor Yellow
$buildResult = & cmake --build $testBuildDir --config Release --target themis_core -j 8 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Build successful!" -ForegroundColor Green
} else {
    Write-Host "❌ Build failed" -ForegroundColor Red
    Write-Host "`nFirst 20 errors:" -ForegroundColor Yellow
    $buildResult | Select-String -Pattern "error" | Select-Object -First 20 |
        ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
}

# Summary
Write-Host "`n=== Summary ===" -ForegroundColor Cyan
Write-Host "Build Directory: $testBuildDir" -ForegroundColor Gray
if (Test-Path "$testBuildDir/CMakeCache.txt") {
    Write-Host "✅ CMake Cache: Created" -ForegroundColor Green
} else {
    Write-Host "❌ CMake Cache: Missing" -ForegroundColor Red
}

if (Test-Path "$testBuildDir/src") {
    $binaries = Get-ChildItem -Path "$testBuildDir/src" -Recurse -Include "*.exe", "*.dll", "*.lib" -ErrorAction SilentlyContinue
    if ($binaries) {
        Write-Host "✅ Build Artifacts: $($binaries.Count) files" -ForegroundColor Green
        $binaries | Select-Object -First 5 | ForEach-Object {
            Write-Host "   - $($_.Name)" -ForegroundColor Gray
        }
    }
}

Write-Host "`n📊 Next Steps:" -ForegroundColor Yellow
Write-Host "  1. Review build log above for errors"
Write-Host "  2. Check $testBuildDir for artifacts"
Write-Host "  3. If successful, push to GitHub for full CI test"
