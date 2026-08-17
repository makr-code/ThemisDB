param()

$in = "scripts/ai_move_candidates.txt"
$out = "scripts/ai_move_filtered.txt"
$pattern = '(?i)(\bissue\b|issue_|\breport\b|phase|gap|gaps|analysis|scan|closure|summary|verification|implementation|remediation|validation)'

if (-not (Test-Path $in)) {
    Write-Error "Input file not found: $in"
    exit 1
}

Get-Content $in |
    Where-Object { $_ -match $pattern } |
    Sort-Object |
    Set-Content $out

$cnt = (Get-Content $out | Measure-Object).Count
Write-Output "Filtered count: $cnt"
if ($cnt -gt 0) {
    Write-Output "--- Sample (first 50) ---"
    Get-Content $out | Select-Object -First 50 | ForEach-Object { Write-Output $_ }
}
