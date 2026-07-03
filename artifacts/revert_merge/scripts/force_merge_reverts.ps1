# Force-resolve revert PRs by merging origin/develop with -s ours so revert branch wins
# Usage: powershell -ExecutionPolicy Bypass -File .\scripts\force_merge_reverts.ps1

$report = @()
$owner = 'makr-code'
$repo = 'ThemisDB'

Write-Output "Loading pr_overview.json..."
$overviewPath = Join-Path $PSScriptRoot '..\pr_overview.json'
if (Test-Path $overviewPath) {
    $overview = Get-Content $overviewPath -Raw | ConvertFrom-Json
} else {
    # fallback to conflict_comment_report.json created earlier
    $fallback = Join-Path $PSScriptRoot '..\conflict_comment_report.json'
    if (Test-Path $fallback) {
        Write-Output "Using fallback $fallback"
        $overview = Get-Content $fallback -Raw | ConvertFrom-Json
    } else {
        Write-Error "Neither pr_overview.json nor conflict_comment_report.json found."
        exit 1
    }
}

# Ensure we have up-to-date origin refs
git fetch origin --prune

foreach ($item in $overview) {
    if ($item.type -ne 'revert' -or [string]::IsNullOrWhiteSpace($item.pr)) { continue }
    $entry = [PSCustomObject]@{
        branch = $item.branch
        prUrl = $item.pr
        prNumber = $null
        success = $false
        stepOutputs = @()
        error = $null
    }

    try {
        $prNumber = ($item.pr -split '/')[-1]
        $entry.prNumber = $prNumber

        Write-Output "Processing PR $prNumber (branch: $($item.branch))..."

        # Checkout branch to local (force create/align with origin)
        $cmd = "git checkout -B $($item.branch) origin/$($item.branch)"
        Write-Output $cmd
        $out = iex $cmd 2>&1
        $entry.stepOutputs += @{step='checkout'; output=($out -join "`n")}

        # Merge develop into branch using 'ours' strategy so branch content wins
        $mergeMsg = "Automated merge: prefer revert branch changes (ours) - merging develop into $($item.branch)"
        $cmd = "git merge -s ours origin/develop -m `"$mergeMsg`""
        Write-Output $cmd
        $out = iex $cmd 2>&1
        $entry.stepOutputs += @{step='merge_ours'; output=($out -join "`n")}

        # Push the merge commit
        $cmd = "git push origin $($item.branch) --force-with-lease"
        Write-Output $cmd
        $out = iex $cmd 2>&1
        $entry.stepOutputs += @{step='push'; output=($out -join "`n")}

        # Try to merge PR via gh
        Write-Output "Attempting to merge PR $prNumber via gh..."
        $cmd = "gh pr merge $prNumber --merge --delete-branch --yes"
        Write-Output $cmd
        $out = iex $cmd 2>&1
        $entry.stepOutputs += @{step='gh_merge'; output=($out -join "`n")}

        if ($LASTEXITCODE -eq 0) { $entry.success = $true }

    } catch {
        $entry.error = $_.Exception.Message
        Write-Error "Error processing $($item.branch): $($_.Exception.Message)"
    }

    $report += $entry
}

$reportPath = Join-Path $PSScriptRoot '..\force_merge_report.json'
$report | ConvertTo-Json -Depth 6 | Out-File $reportPath
Write-Output "Wrote report to $reportPath"

# Return non-zero if any failed
$failed = $report | Where-Object { -not $_.success }
if ($failed.Count -gt 0) { Write-Output "Some merges failed. See report."; exit 2 }
Write-Output "All done."
