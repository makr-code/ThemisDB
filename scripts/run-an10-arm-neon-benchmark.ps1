param(
    [string]$BuildDir = "",
    [string]$OutFile = "",
    [double]$MinGbps = 4.0,
    [double]$MinTimeSeconds = 0.2,
    [int]$Repetitions = 3,
    [switch]$AllowNonArmLabel
)

$ErrorActionPreference = "Stop"

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $repoRoot "build-msvc-ninja-release\cmake"
}
if ([string]::IsNullOrWhiteSpace($OutFile)) {
    $OutFile = Join-Path $repoRoot "artifacts\perf_nv\analytics_arm_neon_aggregation.json"
}

$BuildDir = [System.IO.Path]::GetFullPath($BuildDir)
$OutFile = [System.IO.Path]::GetFullPath($OutFile)

$exe = Join-Path $BuildDir "benchmarks\bench_arm_simd.exe"
if (-not (Test-Path $exe)) {
    throw "Benchmark binary not found: $exe"
}

New-Item -ItemType Directory -Path (Split-Path -Parent $OutFile) -Force | Out-Null

$benchFilter = "BM_ARM_Batch_L2_SIMD/.+"
$args = @(
    "--benchmark_filter=$benchFilter",
    "--benchmark_repetitions=$Repetitions",
    "--benchmark_min_time=$($MinTimeSeconds)s",
    "--benchmark_out=$OutFile",
    "--benchmark_out_format=json"
)

Push-Location $BuildDir
try {
    & $exe @args
    if ($LASTEXITCODE -ne 0) {
        throw "bench_arm_simd exited with code $LASTEXITCODE"
    }
} finally {
    Pop-Location
}

$json = Get-Content $OutFile -Raw | ConvertFrom-Json
$rows = @($json.benchmarks | Where-Object {
    $_.run_type -eq "iteration" -and $_.name -like "BM_ARM_Batch_L2_SIMD/*"
})

if ($rows.Count -eq 0) {
    throw "No BM_ARM_Batch_L2_SIMD iteration rows found in $OutFile"
}

$labels = @($rows | ForEach-Object { $_.label } | Select-Object -Unique)
$minMeasuredGbps = ($rows | Measure-Object -Property effective_read_gb_s -Minimum).Minimum
$maxMeasuredGbps = ($rows | Measure-Object -Property effective_read_gb_s -Maximum).Maximum

Write-Host "AN-10 summary"
Write-Host "- rows: $($rows.Count)"
Write-Host "- labels: $($labels -join ', ')"
Write-Host "- effective_read_gb_s min: $([math]::Round($minMeasuredGbps, 3))"
Write-Host "- effective_read_gb_s max: $([math]::Round($maxMeasuredGbps, 3))"

$hasArmNeonLabel = $labels -contains "ARM_NEON_batch"
$meetsGbps = ($minMeasuredGbps -ge $MinGbps)

if ((-not $AllowNonArmLabel) -and (-not $hasArmNeonLabel)) {
    Write-Host "AN-10 FAILED: expected label ARM_NEON_batch, got: $($labels -join ', ')"
    Write-Host "AN10_STATUS=FAILED"
    Write-Host "AN10_REASON=LABEL_MISMATCH"
    exit 2
}

if (-not $meetsGbps) {
    Write-Host "AN-10 FAILED: effective_read_gb_s min $minMeasuredGbps < required $MinGbps"
    Write-Host "AN10_STATUS=FAILED"
    Write-Host "AN10_REASON=GBPS_BELOW_THRESHOLD"
    exit 3
}

if ($AllowNonArmLabel -and (-not $hasArmNeonLabel)) {
    Write-Host "AN-10 PASSED (probe mode): label check skipped (labels: $($labels -join ', '))"
} else {
    Write-Host "AN-10 PASSED: ARM_NEON_batch and effective_read_gb_s >= $MinGbps"
}
Write-Host "AN10_STATUS=PASSED"
Write-Host "AN10_REASON=OK"
exit 0
