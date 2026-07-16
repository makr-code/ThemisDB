param(
    [string]$BuildPreset = "windows-bench-release",
    [string]$InputJson,
    [switch]$UseGatecheckSeries,
    [switch]$FailOnStretch
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-LatestJson {
    param(
        [string]$Root,
        [bool]$SeriesMode
    )

    $benchRoot = Join-Path $Root ("build/" + $BuildPreset + "/bench-results")
    if (-not (Test-Path $benchRoot)) {
        throw "Benchmark directory not found: $benchRoot"
    }

    $pattern = if ($SeriesMode) {
        "bench_vulkan_transfer_gatecheck_*_run*.json"
    } else {
        "bench_vulkan_transfer_scaling_*.json"
    }

    $latest = Get-ChildItem -Path $benchRoot -Recurse -Filter $pattern -File |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (-not $latest) {
        throw "No benchmark JSON found for pattern '$pattern' under $benchRoot"
    }

    return $latest.FullName
}

function Get-MeanValue {
    param(
        [object]$Json,
        [string]$Name
    )

    $entry = $Json.benchmarks | Where-Object { $_.name -eq $Name } | Select-Object -First 1
    if (-not $entry) {
        throw "Benchmark entry not found: $Name"
    }

    return [double]$entry.real_time
}

$root = Split-Path -Parent $PSScriptRoot
$targetJson = if ([string]::IsNullOrWhiteSpace($InputJson)) {
    Get-LatestJson -Root $root -SeriesMode:$UseGatecheckSeries
} else {
    if (Test-Path $InputJson) {
        (Resolve-Path $InputJson).Path
    } else {
        throw "InputJson does not exist: $InputJson"
    }
}

$json = Get-Content -Raw $targetJson | ConvertFrom-Json

$gates = @(
    [pscustomobject]@{
        Name = "VulkanBenchmarkFixture/BufferUploadDownload/262144_mean"
        Label = "1 MB"
        ReleaseGateMs = 6.0
        StretchGateMs = 5.5
    },
    [pscustomobject]@{
        Name = "VulkanBenchmarkFixture/BufferUploadDownload/1048576_mean"
        Label = "4 MB"
        ReleaseGateMs = 18.5
        StretchGateMs = 18.0
    },
    [pscustomobject]@{
        Name = "VulkanBenchmarkFixture/BufferUploadDownload/4194304_mean"
        Label = "16 MB"
        ReleaseGateMs = 70.0
        StretchGateMs = 68.0
    }
)

$results = foreach ($gate in $gates) {
    $value = Get-MeanValue -Json $json -Name $gate.Name
    $releasePass = $value -le $gate.ReleaseGateMs
    $stretchPass = $value -le $gate.StretchGateMs

    [pscustomobject]@{
        Benchmark = $gate.Name
        Size = $gate.Label
        MeanMs = [math]::Round($value, 3)
        ReleaseGateMs = [math]::Round($gate.ReleaseGateMs, 3)
        Release = if ($releasePass) { "PASS" } else { "FAIL" }
        StretchGateMs = [math]::Round($gate.StretchGateMs, 3)
        Stretch = if ($stretchPass) { "PASS" } else { "FAIL" }
    }
}

$releaseFail = @($results | Where-Object { $_.Release -eq "FAIL" })
$stretchFail = @($results | Where-Object { $_.Stretch -eq "FAIL" })

Write-Host "[INFO] Vulkan transfer gate check file: $targetJson" -ForegroundColor Cyan
$results | Format-Table -AutoSize

if ($releaseFail.Count -gt 0) {
    Write-Host "[FAIL] Release gate failure detected." -ForegroundColor Red
    exit 2
}

if ($FailOnStretch -and $stretchFail.Count -gt 0) {
    Write-Host "[FAIL] Stretch gate failure detected with -FailOnStretch." -ForegroundColor Red
    exit 3
}

if ($stretchFail.Count -gt 0) {
    Write-Host "[WARN] Stretch gate failure detected. Release gates still PASS." -ForegroundColor Yellow
    exit 0
}

Write-Host "[OK] All release and stretch gates passed." -ForegroundColor Green
exit 0
