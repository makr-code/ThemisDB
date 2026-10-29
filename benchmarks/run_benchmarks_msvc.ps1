param(
    [string]$Config = "Release",
    [string]$BuildRoot = "C:\\VCC\\themis\\build-msvc",
    [string]$Filter = "",
    [string]$OutRoot = "benchmarks\\benchmark_results"
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false

function Invoke-ProcessCapture {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$Arguments = @()
    )

    $lines = @()
    try {
        if ($Arguments -and $Arguments.Count -gt 0) {
            $lines = & $FilePath @Arguments 2>&1
        } else {
            $lines = & $FilePath 2>&1
        }
    } catch {
        $lines = @($_.Exception.Message)
    }

    $exitCode = $LASTEXITCODE
    if ($null -eq $exitCode) {
        $exitCode = 0
    }

    return [PSCustomObject]@{
        ExitCode = $exitCode
        Text     = @($lines)
    }
}

function Ensure-Dir($path) {
    if (-not (Test-Path -LiteralPath $path)) {
        New-Item -ItemType Directory -Path $path | Out-Null
    }
}

function ConvertFrom-GoogleBenchmarkJson {
    param(
        [Parameter(Mandatory = $true)][string]$JsonPath,
        [Parameter(Mandatory = $true)][string]$SuiteName
    )

    if (-not (Test-Path -LiteralPath $JsonPath)) {
        return @()
    }

    try {
        $json = Get-Content -LiteralPath $JsonPath -Raw | ConvertFrom-Json
    } catch {
        return @()
    }

    if (-not $json -or -not $json.benchmarks) {
        return @()
    }

    $records = @()
    foreach ($item in $json.benchmarks) {
        $name = $item.name
        if (-not $name) { continue }

        $record = [ordered]@{
            schema_version = 2
            benchmark_kind = 'google'
            suite = $SuiteName
            name = $name
            source = $JsonPath
            timestamp_utc = $json.context.date
            category = $null
            unit = if ($item.time_unit) { $item.time_unit } else { 'ns' }
            metrics = [ordered]@{
                iterations = if ($item.iterations) { [double]$item.iterations } else { $null }
                real_time_ns = if ($item.real_time) { [double]$item.real_time } else { $null }
                cpu_time_ns = if ($item.cpu_time) { [double]$item.cpu_time } else { $null }
                bytes_per_second = if ($item.bytes_per_second) { [double]$item.bytes_per_second } else { $null }
                items_per_second = if ($item.items_per_second) { [double]$item.items_per_second } else { $null }
            }
            acceptance = [ordered]@{
                threshold = $null
                operator = $null
                passed = $null
            }
            raw = [ordered]@{
                label = $item.label
                time_unit = $item.time_unit
            }
        }
        $records += [pscustomobject]$record
    }
    return $records
}

function Export-NormalizedBenchmarkSummary {
    param(
        [Parameter(Mandatory = $true)][string]$OutDir,
        [Parameter(Mandatory = $true)][object[]]$Records,
        [Parameter(Mandatory = $true)][string]$SummaryName
    )

    if (-not $Records -or $Records.Count -eq 0) {
        return
    }

    $summaryPath = Join-Path $OutDir $SummaryName
    $Records | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $summaryPath -Encoding UTF8
}

function Test-GoogleBenchmarkOutput {
    param(
        [Parameter(Mandatory = $true)][string]$JsonPath
    )

    if (-not (Test-Path -LiteralPath $JsonPath)) {
        return $false
    }

    try {
        $raw = Get-Content -LiteralPath $JsonPath -Raw
        if (-not $raw) {
            return $false
        }

        $json = $raw | ConvertFrom-Json
        return $null -ne $json.benchmarks -and @($json.benchmarks).Count -gt 0
    } catch {
        return $false
    }

    return $false
}

$BuildDir = $null
$buildCandidates = @(
    $BuildRoot,
    (Join-Path $BuildRoot 'bin'),
    (Join-Path $BuildRoot $Config),
    (Join-Path (Join-Path $BuildRoot 'bin') $Config)
)

foreach ($candidate in $buildCandidates | Select-Object -Unique) {
    if (Test-Path -LiteralPath $candidate) {
        $benchFiles = Get-ChildItem -LiteralPath $candidate -Filter "bench_*.exe" -File -ErrorAction SilentlyContinue
        if ($benchFiles -and $benchFiles.Count -gt 0) {
            $BuildDir = $candidate
            break
        }
    }
}

if (-not $BuildDir) {
    Write-Error "Benchmark build directory not found. Checked: $($buildCandidates -join '; ')"
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
$normalized = @()
foreach ($exe in $exeList) {
    $name = [System.IO.Path]::GetFileNameWithoutExtension($exe)
    Write-Host "Running $name ..."
    $jsonOut = Join-Path $OutDir ("$name.json")
    $csvOut  = Join-Path $OutDir ("$name.csv")
    $logOut  = Join-Path $OutDir ("$name.txt")

    # The benchmark suite follows one contract: Google Benchmark emits JSON.
    # Custom standalone main() paths are intentionally not part of the supported
    # benchmark harness anymore.
    $args = @("--benchmark_counters_tabular=true", "--benchmark_out=$jsonOut", "--benchmark_out_format=json")
    $runResult = Invoke-ProcessCapture -FilePath $exe -Arguments $args

    if ($runResult.ExitCode -ne 0) {
        Write-Warning ("Benchmark {0} exited with code {1}; its output is not accepted as a successful run." -f $name, $runResult.ExitCode)
        continue
    }

    if (-not (Test-GoogleBenchmarkOutput -JsonPath $jsonOut)) {
        Write-Warning ("Benchmark {0} produced no non-empty Google Benchmark JSON." -f $name)
        continue
    }

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
        $googleRecords = ConvertFrom-GoogleBenchmarkJson -JsonPath $jsonOut -SuiteName $name
        $normalized += $googleRecords
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

if ($normalized.Count -gt 0) {
    Export-NormalizedBenchmarkSummary -OutDir $OutDir -Records $normalized -SummaryName 'benchmark_summary.json'
}

if ($ran.Count -eq 0) {
    Write-Error "No benchmark executable produced valid Google Benchmark results. Output: $OutDir"
    exit 1
}

# README
$readme = @(
    "Themis Benchmarks (MSVC)",
    "Timestamp: $Timestamp",
    "BuildDir: $BuildDir",
    "Output Directory: $OutDir",
    "",
    "Canonical benchmark contract:",
    "  - Every benchmark target is a Google Benchmark executable",
    "  - Output is JSON via --benchmark_out_format=json",
    "  - benchmark_summary.json is the normalized aggregate result",
    "",
    "Usage:",
    "  PowerShell> .\\benchmarks\\run_benchmarks_msvc.ps1 -Config bin -Filter 'encrypt|vector'",
    ""
) -join "`r`n"
Set-Content -LiteralPath (Join-Path $OutDir 'README.txt') -Value $readme -Encoding UTF8

Write-Host "✅ Completed. Results in $OutDir" -ForegroundColor Green
