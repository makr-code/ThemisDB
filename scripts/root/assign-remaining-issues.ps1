#!/usr/bin/env pwsh
<#
  Assign remaining ~3800 issues without milestone to version milestones
  Strategy: Distribute based on issue number across all 24 version milestones (v0.9.0 - v2.3.0)
#>

param([switch]$DryRun = $true)

$repo = "makr-code/ThemisDB"

# Fetch version milestones dynamically so new releases are picked up automatically.
$versions = @(gh api repos/$repo/milestones?state=all --paginate --jq '.[] | select(.title | test("^v[0-9]+\\.[0-9]+\\.[0-9]+$")) | .title' |
	Sort-Object { [version]($_.TrimStart('v')) })

if ($versions.Count -eq 0) {
	Write-Host "No version milestones found." -ForegroundColor Red
	exit 1
}

Write-Host "`n╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  Assigning remaining issues without milestone                   ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════════╝`n" -ForegroundColor Cyan
Write-Host "  Loaded $($versions.Count) version milestones from GitHub" -ForegroundColor Gray

# Fetch all issues without milestone
Write-Host "  Fetching issues without milestone..." -ForegroundColor Yellow

$allIssues = @()
$page = 1
$pageSize = 100

while ($true) {
	Write-Host "    Page $page..." -ForegroundColor Gray

	$pageJson = gh api "repos/$repo/issues?milestone=none&state=all&per_page=$pageSize&page=$page" --jq '.[].number'
	$pageIssues = @($pageJson | Where-Object { -not [string]::IsNullOrEmpty($_) })

	if ($pageIssues.Count -eq 0) {
		Write-Host "    Done ($page pages total)" -ForegroundColor Gray
		break
	}

	$allIssues += $pageIssues
	$page++
}

Write-Host "`n  Total issues found: $($allIssues.Count)" -ForegroundColor Green

if ($allIssues.Count -eq 0) {
	Write-Host "`n✓ No issues to assign!" -ForegroundColor Green
	exit 0
}

# Build assignment map
Write-Host "`n  Building distribution map (by issue number modulo)..." -ForegroundColor Yellow

$assignments = @{}
$versionCount = $versions.Count

foreach ($issueNum in $allIssues) {
	# Use modulo to distribute across all versions evenly
	$versionIdx = [int]$issueNum % $versionCount
	$targetVersion = $versions[$versionIdx]

	$assignments[$issueNum] = $targetVersion
}

# Group and display sample distribution
Write-Host "`n  Distribution summary:" -ForegroundColor Yellow
$grouped = @{}
foreach ($entry in $assignments.GetEnumerator()) {
	if (-not $grouped[$entry.Value]) {
		$grouped[$entry.Value] = 0
	}
	$grouped[$entry.Value]++
}

foreach ($v in $versions) {
	$count = if ($grouped[$v]) { $grouped[$v] } else { 0 }
	if ($count -gt 0) {
		Write-Host "    $v → $count issues" -ForegroundColor Gray
	}
}

# Preview for dry-run
if ($DryRun) {
	Write-Host "`n╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
	Write-Host "║  DRY-RUN MODE - Sample assignments:                            ║" -ForegroundColor Cyan
	Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan

	$samples = $allIssues | Get-Random -Count ([Math]::Min(20, $allIssues.Count))
	foreach ($sample in $samples) {
		Write-Host "  #$sample → $($assignments[$sample])" -ForegroundColor Cyan
	}

	Write-Host "`nRun with -DryRun:`$false to apply $($allIssues.Count) assignments.`n" -ForegroundColor Cyan
	exit 0
}

# Apply all assignments
Write-Host "`n╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║  APPLYING ASSIGNMENTS...                                        ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════════════════════════════╝`n" -ForegroundColor Green

$success = 0
$fail = 0
$total = $allIssues.Count
$i = 0

foreach ($issueNum in $allIssues) {
	$i++
	$targetVersion = $assignments[$issueNum]

	if ($i % 100 -eq 0) {
		Write-Host "  Progress: [$i/$total] assigned so far..." -ForegroundColor Gray
	}

	# Assign milestone with retry logic
	$retryCount = 0
	$maxRetries = 3
	$assigned = $false

	while ($retryCount -lt $maxRetries -and -not $assigned) {
		$result = gh issue edit $issueNum --milestone $targetVersion -R $repo 2>&1

		if ($LASTEXITCODE -eq 0) {
			$success++
			$assigned = $true
		} else {
			$retryCount++
			if ($result -match "rate limit") {
				# Rate limit hit, wait longer
				Write-Host "  ⚠ Rate limit hit! Waiting 60 seconds..." -ForegroundColor Yellow
				Start-Sleep -Seconds 60
			} else {
				$fail++
				if ($fail -le 5) {
					Write-Host "    ✗ #$issueNum → $targetVersion FAILED: $result" -ForegroundColor Red
				}
				$assigned = $true # Don't retry non-rate-limit errors
			}
		}
	}

	if (-not $assigned) {
		$fail++
	}

	# Rate limiting: 500ms minimum between requests
	Start-Sleep -Milliseconds 500
}

Write-Host "`n╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║  COMPLETE                                                       ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════════════════════════════╝" -ForegroundColor Green

Write-Host "`n✅ Success: $success" -ForegroundColor Green
Write-Host "❌ Failed:  $fail" -ForegroundColor $(if ($fail -gt 0) { "Red" } else { "Green" })
Write-Host ""