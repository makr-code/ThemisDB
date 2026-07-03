# Force-resolve revert PRs using 'ours' strategy and GH API merge
# Usage: powershell -ExecutionPolicy Bypass -File .\scripts\force_merge_reverts_v2.ps1

$report = @()
$fallback = Join-Path $PSScriptRoot '..\conflict_comment_report.json'
if (-not (Test-Path $fallback)) { Write-Error "conflict_comment_report.json not found"; exit 1 }
$data = Get-Content $fallback -Raw | ConvertFrom-Json

git fetch origin --prune

foreach ($e in $data) {
    $n = $e.number
    $branch = $e.branch
    if (-not $n -or -not $branch) { continue }
    $entry = [PSCustomObject]@{
        number = $n
        branch = $branch
        actions = @()
        merged = $false
        apiResponse = $null
        error = $null
    }
    try {
        Write-Output "\n== Processing PR $n (branch: $branch) =="
        Write-Output "Checkout branch"
        $out = iex "git checkout -B $branch origin/$branch" 2>&1
        $entry.actions += @{step='checkout'; out=($out -join "`n")}

        Write-Output "Merge develop with -s ours"
        $mergeMsg = "Automated merge: prefer revert branch changes (ours) - merging develop into $branch"
        $out = iex "git merge -s ours origin/develop -m `"$mergeMsg`"" 2>&1
        $entry.actions += @{step='merge_ours'; out=($out -join "`n")}

        Write-Output "Push branch (force-with-lease)"
        $out = iex "git push origin $branch --force-with-lease" 2>&1
        $entry.actions += @{step='push'; out=($out -join "`n")}

        Write-Output "Call GH API to merge PR $n"
        $api = iex "gh api -X PUT /repos/makr-code/ThemisDB/pulls/$n/merge -f merge_method=merge" 2>&1
        $entry.actions += @{step='gh_api_merge'; out=($api -join "`n")}

        # try parse API response
        try { $j = $api | ConvertFrom-Json; if ($j.merged -eq $true) { $entry.merged = $true; $entry.apiResponse = $j } }
        catch { $entry.error = 'API parse failed: '+($_.Exception.Message) }

    } catch {
        $entry.error = $_.Exception.Message
        Write-Error ("Error on PR {0}: {1}" -f $n, $entry.error)
    }
    $report += $entry
}

$reportPath = Join-Path $PSScriptRoot '..\force_merge_v2_report.json'
$report | ConvertTo-Json -Depth 6 | Out-File $reportPath
Write-Output "Wrote report to $reportPath"

$failed = $report | Where-Object { -not $_.merged }
if ($failed.Count -gt 0) { Write-Output "Some PRs failed to merge. See report."; exit 2 }
Write-Output "All PRs merged."
