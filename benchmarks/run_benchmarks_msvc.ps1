param(
    [string]$Config = "Release",
    [string]$BuildRoot = "C:\\VCC\\themis\\build-msvc",
    [string]$Filter = "",
    [string]$OutRoot = "benchmarks\\benchmark_results"
)

$ErrorActionPreference = 'Stop'

function Ensure-Dir($path) {
    if (-not (Test-Path -LiteralPath $path)) {
        New-Item -ItemType Directory -Path $path | Out-Null
    }
}

$BuildDir = Join-Path $BuildRoot $Config
if (-not (Test-Path -LiteralPath $BuildDir)) {
    Write-Error "Build directory not found: $BuildDir"
    exit 2
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir
$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$OutDir = Join-Path $RepoRoot (Join-Path $OutRoot $Timestamp)
Ensure-Dir $OutDir

# Collect executables: bench_*.exe plus aggregated themis_benchmarks.exe, benchmark_image_analysis.exe
$exeList = @()
$exeList += Get-ChildItem -LiteralPath $BuildDir -Filter "bench_*.exe" -File | Select-Object -ExpandProperty FullName
$extra = @("themis_benchmarks.exe", "benchmark_image_analysis.exe") | ForEach-Object { Join-Path $BuildDir $_ }
$exeList += $extra | Where-Object { Test-Path -LiteralPath $_ }

if ($Filter -and $Filter.Trim().Length -gt 0) {
    $regex = [regex]::new($Filter, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    $exeList = $exeList | Where-Object { $regex.IsMatch([System.IO.Path]::GetFileName($_)) }
}

if (-not $exeList -or $exeList.Count -eq 0) {
    Write-Warning "No benchmark executables found in $BuildDir"
    exit 0
}

$ran = @()
foreach ($exe in $exeList) {
    $name = [System.IO.Path]::GetFileNameWithoutExtension($exe)
    Write-Host "Running $name ..."
    $jsonOut = Join-Path $OutDir ("$name.json")
    $csvOut  = Join-Path $OutDir ("$name.csv")
    $args = @("--benchmark_counters_tabular=true", "--benchmark_out=$jsonOut", "--benchmark_out_format=json")
    & $exe @args
    if ($LASTEXITCODE -ne 0) {
        Write-Warning ("Benchmark {0} exited with code {1}. Skipping JSON parse." -f $name, $LASTEXITCODE)
        continue
    }

    # Convert JSON to CSV summary
    try {
        $data = Get-Content -LiteralPath $jsonOut -Raw | ConvertFrom-Json
        $rows = @()
        foreach ($b in $data.benchmarks) {
            $row = [PSCustomObject]@{
                benchmark      = $b.name
                iterations     = $b.iterations
                real_time      = $b.real_time
                time_unit      = $b.time_unit
                cpu_time       = $b.cpu_time
                bytes_per_sec  = if ($b.bytes_per_second) { [double]$b.bytes_per_second } else { $null }
                items_per_sec  = if ($b.items_per_second) { [double]$b.items_per_second } else { $null }
                label          = $b.label
            }
            $rows += $row
        }
        $rows | Export-Csv -LiteralPath $csvOut -NoTypeInformation -Encoding UTF8
        $ran += [PSCustomObject]@{ Name = $name; Json = $jsonOut; Csv = $csvOut }
    } catch {
        Write-Warning ("Failed to parse JSON for {0}: {1}" -f $name, $_)
    }
}

# Combined summary
$summaryCsv = Join-Path $OutDir 'summary.csv'
$combined = @()
foreach ($r in $ran) {
    $csvRows = Import-Csv -LiteralPath $r.Csv
    foreach ($row in $csvRows) {
        $row | Add-Member -NotePropertyName benchmark_suite -NotePropertyValue $r.Name
        $combined += $row
    }
}
if ($combined.Count -gt 0) {
    $combined | Export-Csv -LiteralPath $summaryCsv -NoTypeInformation -Encoding UTF8
}

# README
$readme = @(
    "Themis Benchmarks (MSVC)",
    "Timestamp: $Timestamp",
    "BuildDir: $BuildDir",
    "Output Directory: $OutDir",
    "",
    "Usage:",
    "  PowerShell> .\\benchmarks\\run_benchmarks_msvc.ps1 -Config Release -Filter 'encrypt|vector'",
    ""
) -join "`r`n"
Set-Content -LiteralPath (Join-Path $OutDir 'README.txt') -Value $readme -Encoding UTF8

Write-Host "✅ Completed. Results in $OutDir" -ForegroundColor Green
