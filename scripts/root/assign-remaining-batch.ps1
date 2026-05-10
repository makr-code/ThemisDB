#!/usr/bin/env pwsh
<#
  Assign remaining issues with BATCH processing and long delays
  Batches of 50 issues with 10-second delay between batches
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
Write-Host "║  BATCH: Assigning remaining issues (50 per batch, 10s delay)   ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════════╝`n" -ForegroundColor Cyan
Write-Host "  Loaded $($versions.Count) version milestones from GitHub" -ForegroundColor Gray

# Fetch all issues without milestone
Write-Host "  Fetching issues without milestone..." -ForegroundColor Yellow

$allIssues = @()
$page = 1

while ($true) {
	$pageJson = gh api "repos/$repo/issues?milestone=none&state=all&per_page=100&page=$page" --jq '.[].number'
	$pageIssues = @($pageJson | Where-Object { -not [string]::IsNullOrEmpty($_) })

	if ($pageIssues.Count -eq 0) { break }

	$allIssues += $pageIssues
	$page++
}

Write-Host "  Total issues found: $($allIssues.Count)" -ForegroundColor Green

if ($allIssues.Count -eq 0) {
	Write-Host "`n✓ No issues to assign!" -ForegroundColor Green
	exit 0
}

# Build assignment map
Write-Host "  Building distribution map..." -ForegroundColor Yellow

$assignments = @{}
$versionCount = $versions.Count

foreach ($issueNum in $allIssues) {
	$versionIdx = [int]$issueNum % $versionCount
	$targetVersion = $versions[$versionIdx]
	$assignments[$issueNum] = $targetVersion
}

if ($DryRun) {
	Write-Host "`n  Sample assignments:" -ForegroundColor Cyan
	$allIssues | Get-Random -Count 10 | ForEach-Object {
		Write-Host "    #$_ → $($assignments[$_])" -ForegroundColor Cyan
	}
	Write-Host "`nRun with -DryRun:`$false to apply $($allIssues.Count) assignments.`n" -ForegroundColor Cyan
	exit 0
}

# Apply in batches
Write-Host "`n╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║  APPLYING IN BATCHES...                                         ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════════════════════════════╝`n" -ForegroundColor Green

$batchSize = 50
$totalBatches = [Math]::Ceiling($allIssues.Count / $batchSize)
$success = 0
$fail = 0

for ($batchIdx = 0; $batchIdx -lt $totalBatches; $batchIdx++) {
	$start = $batchIdx * $batchSize
	$end = [Math]::Min($start + $batchSize, $allIssues.Count)
	$batchIssues = $allIssues[$start..($end-1)]

	Write-Host "  Batch $($batchIdx + 1)/${totalBatches}: Issues #$($batchIssues[0]) to #$($batchIssues[-1])..." -ForegroundColor Cyan

	foreach ($issueNum in $batchIssues) {
		$targetVersion = $assignments[$issueNum]

		$result = gh issue edit $issueNum --milestone $targetVersion -R $repo 2>&1

		if ($LASTEXITCODE -eq 0) {
			$success++
		} else {
			$fail++
			if ($result -match "rate limit") {
				Write-Host "    ⚠ Rate limit detected at #$issueNum, waiting 30s..." -ForegroundColor Yellow
				Start-Sleep -Seconds 30

				# Retry once
				$result = gh issue edit $issueNum --milestone $targetVersion -R $repo 2>&1
				if ($LASTEXITCODE -eq 0) {
					$success++
					$fail--
				}
			}
		}

		# Small delay between individual requests (200ms)
		Start-Sleep -Milliseconds 200
	}

	# Long delay between batches (10 seconds)
	if ($batchIdx -lt $totalBatches - 1) {
		Write-Host "    ⏳ Waiting 10 seconds before next batch..." -ForegroundColor Gray
		Start-Sleep -Seconds 10
	}
}

Write-Host "`n╔════════════════════════════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║  COMPLETE                                                       ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════════════════════════════╝`n" -ForegroundColor Green

Write-Host "✅ Success: $success" -ForegroundColor Green
Write-Host "❌ Failed:  $fail" -ForegroundColor $(if ($fail -gt 0) { "Red" } else { "Green" })
Write-Host ""