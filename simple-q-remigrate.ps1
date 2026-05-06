#!/usr/bin/env pwsh
<#
  Simple Q-Milestone remigration script
  Since the Q-milestones have 851+100 closed issues, we'll use a direct approach:
  1. Get all milestone numbers
  2. For each Q-milestone, fetch closed+open issues
  3. Remap to version milestones based on heuristic
#>

param([switch]$DryRun = $true)

$repo = "makr-code/ThemisDB"

Write-Host "`n=== Q-Milestone Remigration (Simple Direct Approach) ===" -ForegroundColor Cyan

# Fetch all milestones at once
$milestones = gh api repos/$repo/milestones --paginate --jq '.[] | {number, title}'  `
    | ConvertFrom-Json

# Mapping for Q → Versions
$qToVersions = @{
    "Q1 2027" = @("v2.1.0", "v2.2.0", "v2.3.0")
    "Q2 2026" = @("v1.5.0", "v1.6.0")
    "Q3 2026" = @("v1.7.0", "v1.8.0")
    "Q4 2026" = @("v1.9.0", "v2.0.0")
}

$migrations = @{}

foreach ($q in $qToVersions.Keys) {
    $qMs = $milestones | Where-Object { $_.title -eq $q }
    if (-not $qMs) {
        Write-Host "$q - NOT FOUND" -ForegroundColor Yellow
        continue
    }
    
    Write-Host "`n  Fetching issues (with pagination)..." -ForegroundColor Cyan
    
    # Fetch with pagination to get ALL issues
    $allPages = @()
    $page = 1
    $perPage = 100
    
    while ($true) {
        $pageJson = gh api "repos/$repo/issues?milestone=$($qMs.number)&state=all&per_page=$perPage&page=$page" --jq '.[].number'
        $pageIssues = @($pageJson | Where-Object { -not [string]::IsNullOrEmpty($_) })
        
        if ($pageIssues.Count -eq 0) { break }
        
        $allPages += $pageIssues
        Write-Host "    Page $($page)- $($pageIssues.Count) issues" -ForegroundColor Gray
        $page++
    }
    
    $qIssues = $allPages
    Write-Host "  Total: $($qIssues.Count) issues"
    
    $versions = $qToVersions[$q]
    $processed = 0
    
    foreach ($issueNum in $qIssues) {
        if ([string]::IsNullOrEmpty($issueNum)) { continue }
        
        # Simple heuristic: distribute evenly
        $idx = [int]$($qIssues.IndexOf($issueNum))
        $ratio = if ($qIssues.Count -gt 0) { $idx / $qIssues.Count } else { 0 }
        
        if ($versions.Count -eq 2) {
            $target = if ($ratio -lt 0.5) { $versions[0] } else { $versions[1] }
        } elseif ($versions.Count -eq 3) {
            if ($ratio -lt 0.33) { $target = $versions[0] }
            elseif ($ratio -lt 0.66) { $target = $versions[1] }
            else { $target = $versions[2] }
        } else {
            $target = $versions[0]
        }
        
        $migrations[$issueNum] = $target
        $processed++
    }
    
    Write-Host "  Planned migrations: $processed"
}

Write-Host "`n╔════════════════════════════════════════╗" -ForegroundColor Blue
Write-Host "║  Total migrations planned: $($migrations.Count)              ║" -ForegroundColor Blue
Write-Host "╚════════════════════════════════════════╝" -ForegroundColor Blue

if ($DryRun) {
    Write-Host "`nDRY-RUN MODE - Sample migrations:" -ForegroundColor Yellow
    $migrations.GetEnumerator() | Select-Object -First 20 | ForEach-Object {
        Write-Host "  #$($_.Name) → $($_.Value)"
    }
    Write-Host "`n... and $($migrations.Count - 20) more" -ForegroundColor Gray
    Write-Host "`nRun with -DryRun`$false to apply." -ForegroundColor Yellow
    exit 0
}

Write-Host "`nAPPLYING MIGRATIONS..." -ForegroundColor Green

$successful = 0
$failed = 0
$idx = 0

foreach ($entry in $migrations.GetEnumerator()) {
    $idx++
    $issueNum = $entry.Name
    $target = $entry.Value
    
    Write-Host -NoNewline "[$idx/$($migrations.Count)] #$issueNum → $target ... "
    
    try {
        $_ = gh issue edit $issueNum --milestone $target -R $repo 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "✓" -ForegroundColor Green
            $successful++
        } else {
            Write-Host "✗" -ForegroundColor Red
            $failed++
        }
    } catch {
        Write-Host "✗" -ForegroundColor Red
        $failed++
    }
    
    Start-Sleep -Milliseconds 100
}

Write-Host "`n✓ Complete: $successful successful, $failed failed" -ForegroundColor Green
