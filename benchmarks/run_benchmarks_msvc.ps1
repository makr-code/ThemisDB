param(
    [string]$Config = "Release",
    [string]$BuildRoot = "C:\\VCC\\themis\\build-msvc",
    [string]$Filter = "",
    [string]$BenchmarkFilter = "",
    [string]$MinTime = "0.1s",
    [ValidateRange(1, 1000)][int]$Repetitions = 1,
    [ValidateRange(1, 86400)][int]$TimeoutSeconds = 300,
    [string]$OutRoot = "benchmarks\\benchmark_results"
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false

if ($MinTime -notmatch '^\d+(?:\.\d+)?(?:ns|us|ms|s|m|h)$') {
    throw "MinTime must include a Google Benchmark time unit (for example 0.1s): $MinTime"
}

if (-not ('ThemisBenchmarkProcessMetrics' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public static class ThemisBenchmarkProcessMetrics
{
    [StructLayout(LayoutKind.Sequential)]
    public struct MemoryCountersEx
    {
        public uint Size;
        public uint PageFaultCount;
        public UIntPtr PeakWorkingSetSize;
        public UIntPtr WorkingSetSize;
        public UIntPtr QuotaPeakPagedPoolUsage;
        public UIntPtr QuotaPagedPoolUsage;
        public UIntPtr QuotaPeakNonPagedPoolUsage;
        public UIntPtr QuotaNonPagedPoolUsage;
        public UIntPtr PagefileUsage;
        public UIntPtr PeakPagefileUsage;
        public UIntPtr PrivateUsage;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct IoCounters
    {
        public ulong ReadOperationCount;
        public ulong WriteOperationCount;
        public ulong OtherOperationCount;
        public ulong ReadTransferCount;
        public ulong WriteTransferCount;
        public ulong OtherTransferCount;
    }

    [DllImport("psapi.dll", SetLastError = true)]
    public static extern bool GetProcessMemoryInfo(
        IntPtr process,
        ref MemoryCountersEx counters,
        uint size);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool GetProcessIoCounters(
        IntPtr process,
        out IoCounters counters);
}
'@
}

function Get-ProcessResourceMetrics {
    param(
        [Parameter(Mandatory = $true)][System.Diagnostics.Process]$Process,
        [Parameter(Mandatory = $true)][double]$WallTimeSeconds
    )

    $memoryCounters = New-Object ThemisBenchmarkProcessMetrics+MemoryCountersEx
    $memoryCounters.Size = [uint][System.Runtime.InteropServices.Marshal]::SizeOf($memoryCounters)
    $hasMemoryMetrics = [ThemisBenchmarkProcessMetrics]::GetProcessMemoryInfo(
        $Process.Handle,
        [ref]$memoryCounters,
        $memoryCounters.Size
    )

    $ioCounters = New-Object ThemisBenchmarkProcessMetrics+IoCounters
    $hasIoMetrics = [ThemisBenchmarkProcessMetrics]::GetProcessIoCounters(
        $Process.Handle,
        [ref]$ioCounters
    )

    $cpuTimeSeconds = $Process.TotalProcessorTime.TotalSeconds
    $cpuCorePercent = if ($WallTimeSeconds -gt 0) {
        100.0 * $cpuTimeSeconds / $WallTimeSeconds
    } else {
        $null
    }
    $cpuMachinePercent = if ($null -ne $cpuCorePercent) {
        $cpuCorePercent / [Math]::Max(1, [Environment]::ProcessorCount)
    } else {
        $null
    }

    return [pscustomobject]@{
        wall_time_seconds = $WallTimeSeconds
        cpu_time_seconds = $cpuTimeSeconds
        cpu_core_equivalents_percent = $cpuCorePercent
        cpu_machine_percent = $cpuMachinePercent
        peak_working_set_bytes = if ($hasMemoryMetrics) { $memoryCounters.PeakWorkingSetSize.ToUInt64() } else { $null }
        peak_commit_bytes = if ($hasMemoryMetrics) { $memoryCounters.PeakPagefileUsage.ToUInt64() } else { $null }
        private_bytes_at_exit = if ($hasMemoryMetrics) { $memoryCounters.PrivateUsage.ToUInt64() } else { $null }
        page_fault_count = if ($hasMemoryMetrics) { $memoryCounters.PageFaultCount } else { $null }
        io_read_bytes = if ($hasIoMetrics) { $ioCounters.ReadTransferCount } else { $null }
        io_write_bytes = if ($hasIoMetrics) { $ioCounters.WriteTransferCount } else { $null }
    }
}

function Invoke-ProcessCapture {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$Arguments = @()
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in $Arguments) {
        $startInfo.ArgumentList.Add($argument)
    }

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $lines = @()
    $exitCode = -1
    $timedOut = $false
    $resourceMetrics = $null
    try {
        if (-not $process.Start()) {
            throw "Failed to start process: $FilePath"
        }
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
            $timedOut = $true
            $process.Kill($true)
            $process.WaitForExit()
        }
        $stopwatch.Stop()

        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        $lines = @($stdout -split "`r?`n") + @($stderr -split "`r?`n")
        if ($timedOut) {
            $lines += "Process exceeded the $TimeoutSeconds second timeout and was terminated."
        }
        $exitCode = $process.ExitCode
        $process.Refresh()
        $resourceMetrics = Get-ProcessResourceMetrics -Process $process -WallTimeSeconds $stopwatch.Elapsed.TotalSeconds
    } catch {
        $stopwatch.Stop()
        $lines += $_.Exception.Message
    } finally {
        $process.Dispose()
    }

    return [PSCustomObject]@{
        ExitCode = $exitCode
        TimedOut = $timedOut
        Text     = @($lines)
        Resources = $resourceMetrics
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
        $nanosecondScale = switch ($item.time_unit) {
            'ns' { 1.0 }
            'us' { 1000.0 }
            'ms' { 1000000.0 }
            's' { 1000000000.0 }
            default { throw "Unsupported Google Benchmark time unit: $($item.time_unit)" }
        }

        $record = [ordered]@{
            schema_version = 2
            benchmark_kind = 'google'
            suite = $SuiteName
            name = $name
            source = $JsonPath
            timestamp_utc = $json.context.date
            category = $null
            unit = 'ns'
            metrics = [ordered]@{
                iterations = if ($item.iterations) { [double]$item.iterations } else { $null }
                real_time_ns = if ($null -ne $item.real_time) { [double]$item.real_time * $nanosecondScale } else { $null }
                cpu_time_ns = if ($null -ne $item.cpu_time) { [double]$item.cpu_time * $nanosecondScale } else { $null }
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
        if ($null -eq $json.benchmarks -or @($json.benchmarks).Count -eq 0) {
            return $false
        }
        foreach ($measurement in $json.benchmarks) {
            if ($measurement.error_occurred -or -not $measurement.name -or
                $null -eq $measurement.real_time -or $null -eq $measurement.cpu_time -or
                $measurement.time_unit -notin @('ns', 'us', 'ms', 's')) {
                return $false
            }
        }
        return $true
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

$processor = Get-CimInstance Win32_Processor | Select-Object -First 1
$computer = Get-CimInstance Win32_ComputerSystem
$operatingSystem = Get-CimInstance Win32_OperatingSystem
$revision = & git -C $RepoRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify benchmark source revision.' }
$worktreeChanges = @(& git -C $RepoRoot status --porcelain --untracked-files=normal)
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify benchmark worktree state.' }
$cachePath = Join-Path $BuildRoot 'CMakeCache.txt'
$manifest = [ordered]@{
    schema_version = 1
    timestamp_utc = [DateTime]::UtcNow.ToString('o')
    source_revision = [string]$revision
    worktree_dirty = $worktreeChanges.Count -gt 0
    runner_sha256 = (Get-FileHash $PSCommandPath -Algorithm SHA256).Hash
    build_directory = (Resolve-Path $BuildRoot).Path
    cmake_cache_sha256 = if (Test-Path $cachePath) { (Get-FileHash $cachePath -Algorithm SHA256).Hash } else { $null }
    operating_system = $operatingSystem.Caption
    operating_system_version = $operatingSystem.Version
    architecture = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()
    processor = $processor.Name
    logical_processors = $computer.NumberOfLogicalProcessors
    total_physical_memory_bytes = $computer.TotalPhysicalMemory
    binary_filter = $Filter
    benchmark_filter = $BenchmarkFilter
    minimum_measurement_time = $MinTime
    repetitions = $Repetitions
    timeout_seconds = $TimeoutSeconds
    measurement_scope = 'internal_component_benchmark'
    external_comparison_ready = $false
    resource_scope = 'single executable lifetime including setup, warmup and teardown; excludes child processes and remote servers'
    resource_limitations = 'Process I/O is not physical disk I/O; no GPU memory, energy, disk footprint or operation latency quantiles are collected.'
    AI_Used = $true
    AI_Models = 'unknown'
    AI_Contributor = 'GitHub Copilot'
    Human_Review = $false
    Human_Author = 'unknown'
}

# Collect benchmark executables, including modules whose target names start with benchmark_.
$exeList = @()
$exeList += Get-ChildItem -LiteralPath $BuildDir -Filter "bench_*.exe" -File | Select-Object -ExpandProperty FullName
$exeList += Get-ChildItem -LiteralPath $BuildDir -Filter "benchmark_*.exe" -File | Select-Object -ExpandProperty FullName
$extra = @("themis_benchmarks.exe", "benchmark_image_analysis.exe", "llm_bench.exe") | ForEach-Object { Join-Path $BuildDir $_ }
$exeList += $extra | Where-Object { Test-Path -LiteralPath $_ }
$exeList = @($exeList | Sort-Object -Unique)

if ($Filter -and $Filter.Trim().Length -gt 0) {
    $regex = [regex]::new($Filter, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    $exeList = $exeList | Where-Object { $regex.IsMatch([System.IO.Path]::GetFileName($_)) }
}

if (-not $exeList -or $exeList.Count -eq 0) {
    Write-Warning "No benchmark executables found in $BuildDir"
    exit 2
}

$manifest['executables'] = @($exeList | ForEach-Object {
    [ordered]@{
        name = [System.IO.Path]::GetFileName($_)
        sha256 = (Get-FileHash $_ -Algorithm SHA256).Hash
    }
})
$manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $OutDir 'run_manifest.json') -Encoding UTF8

$ran = @()
$normalized = @()
$executionSummary = @()
$resourceSummary = @()
foreach ($exe in $exeList) {
    if ($executionSummary.Count -gt 0) {
        $executionSummary | Export-Csv -LiteralPath (Join-Path $OutDir 'execution_summary.csv') -NoTypeInformation -Encoding UTF8
    }
    if ($resourceSummary.Count -gt 0) {
        $resourceSummary | Export-Csv -LiteralPath (Join-Path $OutDir 'resource_summary.csv') -NoTypeInformation -Encoding UTF8
    }
    $name = [System.IO.Path]::GetFileNameWithoutExtension($exe)
    Write-Host "Running $name ..."
    $jsonOut = Join-Path $OutDir ("$name.json")
    $csvOut  = Join-Path $OutDir ("$name.csv")
    $logOut  = Join-Path $OutDir ("$name.txt")

    # The benchmark suite follows one contract: Google Benchmark emits JSON.
    # Custom standalone main() paths are intentionally not part of the supported
    # benchmark harness anymore.
    $benchmarkArgs = @(
        "--benchmark_counters_tabular=true",
        "--benchmark_min_time=$MinTime",
        "--benchmark_repetitions=$Repetitions",
        "--benchmark_out=$jsonOut",
        "--benchmark_out_format=json"
    )
    if ($BenchmarkFilter) {
        $benchmarkArgs += "--benchmark_filter=$BenchmarkFilter"
    }
    $runResult = Invoke-ProcessCapture -FilePath $exe -Arguments $benchmarkArgs
    $logLines = @(
        "Executable: $exe",
        "Arguments: $($benchmarkArgs -join ' ')",
        "ExitCode: $($runResult.ExitCode)",
        "TimedOut: $($runResult.TimedOut)",
        "PeakWorkingSetBytes: $($runResult.Resources.peak_working_set_bytes)",
        "PeakCommitBytes: $($runResult.Resources.peak_commit_bytes)",
        "CpuTimeSeconds: $($runResult.Resources.cpu_time_seconds)",
        "IoReadBytes: $($runResult.Resources.io_read_bytes)",
        "IoWriteBytes: $($runResult.Resources.io_write_bytes)",
        ""
    ) + @($runResult.Text | ForEach-Object { [string]$_ })
    [System.IO.File]::WriteAllLines(
        $logOut,
        [string[]]$logLines,
        [System.Text.UTF8Encoding]::new($false)
    )
    if ($runResult.Resources) {
        $resourceSummary += [pscustomobject]@{
            executable = $name
            benchmark_exit_code = $runResult.ExitCode
            benchmark_json_valid = $false
            wall_time_seconds = $runResult.Resources.wall_time_seconds
            cpu_time_seconds = $runResult.Resources.cpu_time_seconds
            cpu_core_equivalents_percent = $runResult.Resources.cpu_core_equivalents_percent
            cpu_machine_percent = $runResult.Resources.cpu_machine_percent
            peak_working_set_bytes = $runResult.Resources.peak_working_set_bytes
            peak_commit_bytes = $runResult.Resources.peak_commit_bytes
            private_bytes_at_exit = $runResult.Resources.private_bytes_at_exit
            page_fault_count = $runResult.Resources.page_fault_count
            io_read_bytes = $runResult.Resources.io_read_bytes
            io_write_bytes = $runResult.Resources.io_write_bytes
        }
    }

    if ($runResult.TimedOut -or $runResult.ExitCode -ne 0) {
        Write-Warning ("Benchmark {0} exited with code {1}; its output is not accepted as a successful run." -f $name, $runResult.ExitCode)
        $executionSummary += [pscustomobject]@{
            executable = $name
            exit_code = $runResult.ExitCode
            json_valid = $false
            benchmark_rows = 0
            status = if ($runResult.TimedOut) { 'timeout' } else { 'exit_nonzero' }
        }
        continue
    }

    if (-not (Test-GoogleBenchmarkOutput -JsonPath $jsonOut)) {
        Write-Warning ("Benchmark {0} produced invalid, empty or error-marked Google Benchmark JSON." -f $name)
        $executionSummary += [pscustomobject]@{
            executable = $name
            exit_code = $runResult.ExitCode
            json_valid = $false
            benchmark_rows = 0
            status = 'invalid_or_empty_json'
        }
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
        $executionSummary += [pscustomobject]@{
            executable = $name
            exit_code = $runResult.ExitCode
            json_valid = $true
            benchmark_rows = $rows.Count
            status = 'success'
        }
        if ($resourceSummary.Count -gt 0) {
            $resourceSummary[-1].benchmark_json_valid = $true
        }
    } catch {
        Write-Warning ("Failed to parse JSON for {0}: {1}" -f $name, $_)
        Add-Content -LiteralPath $logOut -Value ("`nJSON parse error: {0}" -f $_) -Encoding UTF8
        $executionSummary += [pscustomobject]@{
            executable = $name
            exit_code = $runResult.ExitCode
            json_valid = $false
            benchmark_rows = 0
            status = 'json_parse_error'
        }
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

if ($executionSummary.Count -gt 0) {
    $executionSummary | Export-Csv -LiteralPath (Join-Path $OutDir 'execution_summary.csv') -NoTypeInformation -Encoding UTF8
}
if ($resourceSummary.Count -gt 0) {
    $resourceSummary | Export-Csv -LiteralPath (Join-Path $OutDir 'resource_summary.csv') -NoTypeInformation -Encoding UTF8
}

# README
$readme = @(
    "Themis Benchmarks (MSVC)",
    "Timestamp: $Timestamp",
    "BuildDir: $BuildDir",
    "Output Directory: $OutDir",
    "Executables discovered: $($exeList.Count)",
    "Valid Google Benchmark executables: $($ran.Count)",
    "Executables without valid results: $($executionSummary.Count - $ran.Count)",
    "Google Benchmark min time: $MinTime",
    "Google Benchmark repetitions: $Repetitions",
    "Per-executable timeout seconds: $TimeoutSeconds",
    "Run context: run_manifest.json (internal component benchmarks; not external standard compliance)",
    "Process resource metrics: resource_summary.csv (CPU, peak memory, page faults, process I/O)",
    "",
    "Canonical benchmark contract:",
    "  - Every benchmark target is a Google Benchmark executable",
    "  - Output is JSON via --benchmark_out_format=json",
    "  - benchmark_summary.json is the normalized aggregate result",
    "",
    "Usage:",
    "  PowerShell> .\\benchmarks\\run_benchmarks_msvc.ps1 -BuildRoot .\\build-msvc-windows-release -MinTime 0.1s -Filter 'encrypt|vector'",
    ""
) -join "`r`n"
Set-Content -LiteralPath (Join-Path $OutDir 'README.txt') -Value $readme -Encoding UTF8

Write-Host "Completed: $($ran.Count)/$($exeList.Count) valid suites. Results in $OutDir"
if ($executionSummary | Where-Object { $_.status -ne 'success' }) {
    exit 1
}
