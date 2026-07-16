param(
    [string]$ArtifactDir = ''
)
if (-not $ArtifactDir) {
    $ArtifactDir = (Get-ChildItem -Path artifacts\bench_release -Directory | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
}
if (-not $ArtifactDir) { Write-Error "No artifact directory found under artifacts/bench_release"; exit 2 }
$logDir = Join-Path $ArtifactDir 'logs'
if (-not (Test-Path $logDir)) { Write-Error "Log directory not found: $logDir"; exit 3 }

Write-Host "Analyzing logs in: $logDir"`n
# Collect microbench lines
$pattern = 'SelfRAG retrieval microbench: iters=(\d+) p50=(\d+) p95=(\d+) p99=(\d+) mean=(\d+)'
$p50s = New-Object System.Collections.Generic.List[double]
$p95s = New-Object System.Collections.Generic.List[double]
$p99s = New-Object System.Collections.Generic.List[double]
$means = New-Object System.Collections.Generic.List[double]

Get-ChildItem -Path $logDir -Filter *.log -File | ForEach-Object {
    $file = $_.FullName
    Select-String -Path $file -Pattern $pattern | ForEach-Object {
        $m = [regex]::Match($_.Line,$pattern)
        if ($m.Success) {
            $p50s.Add([double]$m.Groups[2].Value)
            $p95s.Add([double]$m.Groups[3].Value)
            $p99s.Add([double]$m.Groups[4].Value)
            $means.Add([double]$m.Groups[5].Value)
        }
    }
}

$GetPercentile = {
    param($arr, $p)
    if (-not $arr -or $arr.Length -eq 0) { return $null }
    $sorted = $arr | Sort-Object
    $len = $sorted.Length
    $idx = [math]::Ceiling($len * $p) - 1
    if ($idx -lt 0) { $idx = 0 }
    if ($idx -ge $len) { $idx = $len - 1 }
    return $sorted[$idx]
}

Write-Host "Microbench samples: p50_count=$($p50s.Count), p95_count=$($p95s.Count), p99_count=$($p99s.Count)`n"
if ($p50s.Count -gt 0) {
    $p50_arr = $p50s.ToArray()
    $p95_arr = $p95s.ToArray()
    $p99_arr = $p99s.ToArray()
    $means_arr = $means.ToArray()
    $p50_stats = $p50_arr | Measure-Object -Minimum -Maximum -Average
    $p95_stats = $p95_arr | Measure-Object -Minimum -Maximum -Average
    $p99_stats = $p99_arr | Measure-Object -Minimum -Maximum -Average
    $mean_stats = $means_arr | Measure-Object -Minimum -Maximum -Average
    $p50_p50 = & $GetPercentile $p50_arr 0.5
    $p50_p95 = & $GetPercentile $p50_arr 0.95
    $p50_p99 = & $GetPercentile $p50_arr 0.99
    $p95_p50 = & $GetPercentile $p95_arr 0.5
    $p95_p95 = & $GetPercentile $p95_arr 0.95
    $p95_p99 = & $GetPercentile $p95_arr 0.99
    $p99_p50 = & $GetPercentile $p99_arr 0.5
    $p99_p95 = & $GetPercentile $p99_arr 0.95
    $p99_p99 = & $GetPercentile $p99_arr 0.99

    $p50_summary = @{min=($p50_stats.Minimum); max=($p50_stats.Maximum); mean=([math]::Round($p50_stats.Average,2)); p50=$p50_p50; p95=$p50_p95; p99=$p50_p99}
    $p95_summary = @{min=($p95_stats.Minimum); max=($p95_stats.Maximum); mean=([math]::Round($p95_stats.Average,2)); p50=$p95_p50; p95=$p95_p95; p99=$p95_p99}
    $p99_summary = @{min=($p99_stats.Minimum); max=($p99_stats.Maximum); mean=([math]::Round($p99_stats.Average,2)); p50=$p99_p50; p95=$p99_p95; p99=$p99_p99}
    $mean_summary = @{min=($mean_stats.Minimum); max=($mean_stats.Maximum); mean=([math]::Round($mean_stats.Average,2))}

    Write-Host "p50 per-occurrence stats (ns): min=$($p50_summary.min) mean=$($p50_summary.mean) p50=$($p50_summary.p50) p95=$($p50_summary.p95) p99=$($p50_summary.p99) max=$($p50_summary.max)"
    Write-Host "p95 per-occurrence stats (ns): min=$($p95_summary.min) mean=$($p95_summary.mean) p50=$($p95_summary.p50) p95=$($p95_summary.p95) p99=$($p95_summary.p99) max=$($p95_summary.max)"
    Write-Host "p99 per-occurrence stats (ns): min=$($p99_summary.min) mean=$($p99_summary.mean) p50=$($p99_summary.p50) p95=$($p99_summary.p95) p99=$($p99_summary.p99) max=$($p99_summary.max)"
    Write-Host "Means per-occurrence (ns): min=$($mean_summary.min) mean=$($mean_summary.mean) max=$($mean_summary.max)`n"
} else {
    Write-Host "No microbench samples found."
}

# Extract backtraces
$outBacktraceFile = Join-Path $ArtifactDir 'backtraces.txt'
$out = New-Object System.Text.StringBuilder
$found = 0
Get-ChildItem -Path $logDir -Filter *.log -File | ForEach-Object {
    $lines = Get-Content -Path $_.FullName -Raw -ErrorAction SilentlyContinue -Encoding UTF8
    $rx = New-Object System.Text.RegularExpressions.Regex "=== Backtrace \(retrieval_slow\):.*?=== End Backtrace ===", ([System.Text.RegularExpressions.RegexOptions]::Singleline)
    $matches = $rx.Matches($lines)
    foreach ($m in $matches) {
        $bt = $m.Value.Trim()
        # deduplicate
        if ($out.ToString().Contains($bt)) { continue }
        $out.AppendLine($bt) | Out-Null
        $out.AppendLine('') | Out-Null
        $found++
        if ($found -ge 10) { break }
    }
    if ($found -ge 10) { break }
}

if ($found -gt 0) {
    $out.ToString() | Out-File -FilePath $outBacktraceFile -Encoding utf8
    Write-Host "Extracted $found backtrace(s) to: $outBacktraceFile`n"
} else {
    Write-Host "No retrieval_slow backtraces found."
}

# Short summary of slow occurrences per log (counts)
Write-Host "Slow-backtrace counts per log (top 10):"
Get-ChildItem -Path $logDir -Filter *.log -File | ForEach-Object {
    $cnt = Select-String -Path $_.FullName -Pattern '=== Backtrace \(retrieval_slow\):' -SimpleMatch | Measure-Object | Select-Object -ExpandProperty Count
    [PSCustomObject]@{file=$_.Name; count=$cnt}
} | Sort-Object -Property count -Descending | Select-Object -First 10 | Format-Table -AutoSize

Write-Host "Analysis complete."