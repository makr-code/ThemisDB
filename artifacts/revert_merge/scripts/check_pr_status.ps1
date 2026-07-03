# Check PR status for PRs listed in conflict_comment_report.json
$data = Get-Content (Join-Path $PSScriptRoot '..\conflict_comment_report.json') -Raw | ConvertFrom-Json
foreach ($e in $data) {
    $n = $e.number
    if (-not $n) { continue }
    Write-Output "== PR $n =="
    try {
        gh pr view $n --json number,state,mergeable,mergeStateStatus,mergedAt | ConvertFrom-Json | ConvertTo-Json -Depth 4
    } catch {
        Write-Output ("gh pr view failed for {0}: {1}" -f $n, $_)
    }
}
