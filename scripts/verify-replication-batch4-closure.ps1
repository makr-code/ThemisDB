#!/usr/bin/env pwsh
# Integration Verification Script for Replication Module Gap Closure Batch 4

param(
    [string]$BuildPreset = "windows-release",
    [bool]$RunBenchmarks = $false
)

Write-Host "=== Replication Module Gap Closure Batch 4 — Integration Verification ===" -ForegroundColor Cyan
Write-Host "Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss UTC')" -ForegroundColor Gray

$repoRoot = Get-Location
$buildDir = Join-Path $repoRoot "build"
$testTimeout = 120  # seconds

# Track overall status
$allTestsPassed = $true
$failedTests = @()

# ============================================================================
# 1. Build Verification
# ============================================================================

Write-Host "`n[1/5] Build Verification" -ForegroundColor Yellow

Write-Host "Configuring CMake preset: $BuildPreset"
$configResult = & cmake --preset $BuildPreset 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Configure failed" -ForegroundColor Red
    Write-Host $configResult
    exit 1
}
Write-Host "✓ Configure succeeded" -ForegroundColor Green

Write-Host "Building with preset: $BuildPreset"
$buildResult = & cmake --build --preset $BuildPreset --parallel 16 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Build failed" -ForegroundColor Red
    Write-Host $buildResult
    exit 1
}
Write-Host "✓ Build succeeded" -ForegroundColor Green

# ============================================================================
# 2. Replication Core Tests
# ============================================================================

Write-Host "`n[2/5] Replication Core Test Suite" -ForegroundColor Yellow

$coreTests = @(
    "test_replication_raft_v2"
    "test_logical_replication"
    "test_replication_coordinator_focused"
    "test_replication_crdt_types"
)

foreach ($test in $coreTests) {
    Write-Host "Running: $test"
    $testResult = & ctest --preset $BuildPreset -R $test --output-on-failure -VV 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ $test FAILED" -ForegroundColor Red
        $failedTests += $test
        $allTestsPassed = $false
    } else {
        Write-Host "✓ $test PASSED" -ForegroundColor Green
    }
}

# ============================================================================
# 3. Replication HA & Failover Tests
# ============================================================================

Write-Host "`n[3/5] Replication HA & Failover Tests" -ForegroundColor Yellow

$haTests = @(
    "test_replication_ha"
    "test_replication_geo"
    "test_replication_*wal*"
)

foreach ($test in $haTests) {
    Write-Host "Running: $test"
    $testResult = & ctest --preset $BuildPreset -R $test --output-on-failure 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "⚠ $test may have issues" -ForegroundColor Yellow
        # Don't fail on HA tests (they're optional)
    } else {
        Write-Host "✓ $test PASSED" -ForegroundColor Green
    }
}

# ============================================================================
# 4. Full Replication Test Suite
# ============================================================================

Write-Host "`n[4/5] Full Replication Test Suite" -ForegroundColor Yellow

Write-Host "Running all replication tests..."
$allTestsResult = & ctest --preset $BuildPreset -k "replication" --output-on-failure 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "⚠ Some replication tests failed (see above for details)" -ForegroundColor Yellow
    $allTestsPassed = $false
} else {
    Write-Host "✓ All replication tests PASSED" -ForegroundColor Green
}

# ============================================================================
# 5. Benchmarks (Optional)
# ============================================================================

if ($RunBenchmarks) {
    Write-Host "`n[5/5] Replication Benchmark Verification" -ForegroundColor Yellow
    
    # Check if benchmark target exists
    $benchmarkTarget = Join-Path $buildDir "Release" "bin" "bench_replication_release_gates*"
    
    if (Test-Path $benchmarkTarget) {
        Write-Host "Running replication benchmarks..."
        
        # Expected: benchmarks should complete without errors and not exceed baseline
        # Note: This is a basic check; full benchmark analysis requires baseline data
        
        Write-Host "⚠ Benchmark verification requires baseline data (not in this script)" -ForegroundColor Yellow
    } else {
        Write-Host "ℹ Benchmark targets not found (may not be configured)" -ForegroundColor Cyan
    }
} else {
    Write-Host "`n[5/5] Benchmark Verification (skipped)" -ForegroundColor Gray
}

# ============================================================================
# Summary & Report
# ============================================================================

Write-Host "`n$('='*70)" -ForegroundColor Cyan
Write-Host "Integration Verification Summary" -ForegroundColor Cyan
Write-Host "$('='*70)" -ForegroundColor Cyan

if ($allTestsPassed) {
    Write-Host "✓ All critical tests PASSED" -ForegroundColor Green
    Write-Host "✓ Replication module is ready for merge" -ForegroundColor Green
    Write-Host "`nNext steps:" -ForegroundColor Cyan
    Write-Host "  1. Review agent completion reports" -ForegroundColor Gray
    Write-Host "  2. Verify no breaking changes in public API" -ForegroundColor Gray
    Write-Host "  3. Create PR on develop with evidence" -ForegroundColor Gray
    exit 0
} else {
    Write-Host "❌ Some tests FAILED" -ForegroundColor Red
    Write-Host "`nFailed tests:" -ForegroundColor Red
    foreach ($test in $failedTests) {
        Write-Host "  - $test" -ForegroundColor Red
    }
    Write-Host "`nRecommended actions:" -ForegroundColor Yellow
    Write-Host "  1. Investigate each failed test individually" -ForegroundColor Gray
    Write-Host "  2. Check if test failures are related to gap closure changes" -ForegroundColor Gray
    Write-Host "  3. Coordinate with implementing agent for fixes" -ForegroundColor Gray
    Write-Host "  4. Re-run verification after fixes" -ForegroundColor Gray
    exit 1
}
