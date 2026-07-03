# Attempt GH API merge for still-open revert PRs from conflict_comment_report.json
$fallback = Join-Path $PSScriptRoot '..\conflict_comment_report.json'
if (-not (Test-Path $fallback)) { Write-Error "conflict_comment_report.json not found"; exit 1 }
$data = Get-Content $fallback -Raw | ConvertFrom-Json
$results = @()

foreach ($e in $data) {
    $n = $e.number; $branch = $e.branch
    if (-not $n) { continue }
    Write-Output "Checking PR $n (branch: $branch)"
    try {
        $pr = gh pr view $n --json number,state,mergeable,mergeStateStatus,mergedAt 2>$null | ConvertFrom-Json
    } catch {
        Write-Output ("gh pr view failed for {0}: {1}" -f $n, $_)
        continue
    }
    if ($pr.state -ne 'OPEN') { Write-Output "PR $n state: $($pr.state) - skipping"; continue }
    Write-Output "PR $n mergeable: $($pr.mergeable) status: $($pr.mergeStateStatus)"
    Write-Output "Attempting API merge for $n"
    try {
        $apiOut = gh api -X PUT "/repos/makr-code/ThemisDB/pulls/$n/merge" -f merge_method=merge 2>&1
        Write-Output $apiOut
        $results += @{number=$n; result=$apiOut -join "`n"}
    } catch {
        Write-Output ("API merge error for {0}: {1}" -f $n, $_)
        $results += @{number=$n; error=$_.Exception.Message}
    }
    Start-Sleep -Milliseconds 500
}
$outPath = Join-Path $PSScriptRoot '..\attempt_merge_remaining_report.json'
$results | ConvertTo-Json -Depth 6 | Out-File $outPath
Write-Output "Wrote $outPath"
