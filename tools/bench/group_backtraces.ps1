param(
    [string]$ArtifactDir = ''
)
if (-not $ArtifactDir) {
    $ArtifactDir = (Get-ChildItem -Path artifacts\bench_release -Directory | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
}
if (-not $ArtifactDir) { Write-Error "No artifact directory found under artifacts/bench_release"; exit 2 }
$logDir = Join-Path $ArtifactDir 'logs'
if (-not (Test-Path $logDir)) { Write-Error "Log directory not found: $logDir"; exit 3 }

Write-Host "Grouping retrieval_slow backtraces in: $logDir`n"

$pattern = '=== Backtrace \(retrieval_slow\):.*?=== End Backtrace ==='
$rx = New-Object System.Text.RegularExpressions.Regex $pattern, ([System.Text.RegularExpressions.RegexOptions]::Singleline)

$htable = @{}
$files = Get-ChildItem -Path $logDir -Filter *.log -File
foreach ($f in $files) {
    try {
        $s = Get-Content -Path $f.FullName -Raw -ErrorAction Stop -Encoding UTF8
    } catch {
        Write-Warning "Failed to read $($f.FullName): $_"
        continue
    }
    $matches = $rx.Matches($s)
    foreach ($m in $matches) {
        $bt_raw = $m.Value
        # normalize: trim each line and join with \n to get consistent keys
        $lines = $bt_raw -split "\r?\n" | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' }
        $bt = $lines -join "`n"
        if ($htable.ContainsKey($bt)) { $htable[$bt] = $htable[$bt] + 1 } else { $htable[$bt] = 1 }
    }
}

if ($htable.Count -eq 0) {
    Write-Host "No retrieval_slow backtraces found.`n"
    exit 0
}

$groupedPath = Join-Path $ArtifactDir 'backtraces_grouped.txt'
$topPath = Join-Path $ArtifactDir 'backtraces_top10.txt'

$sbAll = New-Object System.Text.StringBuilder
$sbTop = New-Object System.Text.StringBuilder

$entries = $htable.GetEnumerator() | Sort-Object Value -Descending
$count = 0
foreach ($e in $entries) {
    $count++
    $block = "Count=$($e.Value)`n$($e.Key)`n`n"
    $sbAll.AppendLine($block) | Out-Null
    if ($count -le 10) { $sbTop.AppendLine($block) | Out-Null }
}

$sbAll.ToString() | Out-File -FilePath $groupedPath -Encoding utf8
$sbTop.ToString() | Out-File -FilePath $topPath -Encoding utf8

Write-Host "Wrote grouped backtraces to: $groupedPath"
Write-Host "Wrote top-10 backtraces to: $topPath`n"

Write-Host "Top-10 (by count):`n"
$entries | Select-Object -First 10 | ForEach-Object { Write-Host "Count=$($_.Value)`n$($_.Key.Substring(0,[math]::Min(200,$_.Key.Length)))`n" }

Write-Host "Done.`n"
