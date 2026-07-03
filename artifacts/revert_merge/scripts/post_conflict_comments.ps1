Param()
$prs = @(5532,5530,5540,5531,5535,5533,5536,5538,5541,5539,5542,5537,5534)
$owner='makr-code'; $repo='ThemisDB'
$report = @()
foreach ($n in $prs) {
  Write-Output "\n== PR $n =="
  $entry = [PSCustomObject]@{number=$n; branch=$null; mergeable=$null; mergeable_state=$null; pr_files=@(); conflicted=@(); commentPosted=$false; commentUrl=$null; error=$null}
  try {
    $prJson = gh api /repos/$owner/$repo/pulls/$n 2>&1 | Out-String
    $pr = $prJson | ConvertFrom-Json
  } catch { $entry.error = 'gh api pr fetch failed: '+$_.Exception.Message; $report += $entry; Write-Output $entry; continue }
  $entry.branch = $pr.head.ref
  $entry.mergeable = $pr.mergeable
  $entry.mergeable_state = $pr.mergeable_state
  # get pr files
  try {
    $filesJson = gh api /repos/$owner/$repo/pulls/$n/files --paginate 2>&1 | Out-String
    $files = $filesJson | ConvertFrom-Json
    $entry.pr_files = ($files | ForEach-Object { $_.filename }) -join "; "
  } catch { $entry.pr_files = 'failed to list files: '+$_.Exception.Message }

  # local merge simulation to find conflict files
  try {
    git fetch origin --prune 2>$null | Out-Null
    git checkout $entry.branch 2>$null | Out-Null
    git reset --hard origin/$($entry.branch) 2>$null | Out-Null
    git fetch origin develop 2>$null | Out-Null
    $mergeOutput = git merge --no-commit --no-ff origin/develop 2>&1
    $mergeExit = $LASTEXITCODE
    if ($mergeExit -ne 0) {
      $conflicts = git diff --name-only --diff-filter=U 2>$null | Select-Object -Unique
      $entry.conflicted = $conflicts -join '; '
      # abort merge state if present
      git merge --abort 2>$null | Out-Null
    } else {
      # clean up merge if successful (reset back)
      git reset --hard HEAD~1 2>$null | Out-Null
    }
    $entry.actions = @{mergeExit=$mergeExit; mergeOutput=($mergeOutput -join "`n")}
  } catch { $entry.error = 'local merge simulation failed: '+$_.Exception.Message }

  # build comment with here-string to avoid escaping issues
  $shortOut = ''
  if ($entry.actions -and $entry.actions.mergeOutput) {
    $lines = $entry.actions.mergeOutput -split "`n"
    $shortOut = ($lines[0..[Math]::Min(9, $lines.Length-1)] -join "`n")
  }
  $conflictText = if ($entry.conflicted -and $entry.conflicted -ne '') { $entry.conflicted } else { 'None detected locally' }
  $comment = @"
Automated conflict report for this revert PR.

- PR: $n
- Branch: $($entry.branch)
- GitHub mergeable: $($entry.mergeable) (state: $($entry.mergeable_state))
- Files changed in PR: $($entry.pr_files)
- Local merge simulation exit: $($entry.actions.mergeExit)

Merge output (truncated):
$shortOut

Detected conflicting files (local): $conflictText

Suggested action: please review the listed files in this PR and resolve conflicts manually or approve the revert. If you want me to attempt a force-resolution that prefers the revert changes, reply and I'll proceed.
"@

  # post comment
  try {
    $resp = gh api -X POST /repos/$owner/$repo/issues/$n/comments -f body="$comment" 2>&1 | Out-String
    # try parse response for url
    try { $j = $resp | ConvertFrom-Json; $entry.commentPosted = $true; $entry.commentUrl = $j.html_url } catch { $entry.commentPosted = $false }
    if ($entry.commentPosted) { Write-Output "Posted comment: $($entry.commentUrl)" } else { Write-Output "Posted comment (no URL returned)" }
  } catch { $entry.error += ' comment failed: '+$_.Exception.Message; Write-Output 'comment failed' }

  $report += $entry
}
$report | ConvertTo-Json -Depth 6 | Out-File c:\Projects\ThemisDB\conflict_comment_report.json -Encoding UTF8
Write-Output "Wrote c:\Projects\ThemisDB\conflict_comment_report.json"
