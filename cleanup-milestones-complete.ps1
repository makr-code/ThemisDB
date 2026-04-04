#!/usr/bin/env pwsh
<#
  Comprehensive Milestone History Cleanup
  1. Create historical milestones (v0.9.0 ~ v1.4.1)
  2. Intelligently assign unassigned issues based on creation date
  3. Migrate Q-milestones to version milestones
#>

param([switch]$DryRun = $true)

$repo = "makr-code/ThemisDB"

Write-Host "`n╔════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  ThemisDB: Complete Milestone History Cleanup              ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════╝`n" -ForegroundColor Cyan

# ============================================================================
# PHASE 1: Create historical milestones
# ============================================================================

$historicalVersions = @(
    "v0.9.0",
    "v1.0.0", "v1.0.1", "v1.0.2",
    "v1.1.0", "v1.1.1", "v1.1.2",
    "v1.2.0", "v1.2.1", "v1.2.2",
    "v1.3.0", "v1.3.1", "v1.3.2",
    "v1.4.0", "v1.4.1"
)

Write-Host "=== PHASE 1: Creating historical milestones ===" -ForegroundColor Green

# Get existing milestones
$existingMs = gh api repos/$repo/milestones --paginate --jq '.[].title'

$createdCount = 0
foreach ($version in $historicalVersions) {
    if ($existingMs -contains $version) {
        Write-Host "  $version - already exists" -ForegroundColor Gray
        continue
    }
    
    Write-Host "  Creating $version... " -NoNewline
    
    if (-not $DryRun) {
        $result = gh api repos/$repo/milestones --method POST `
            -f title=$version `
            -f description="Release $version - Historical milestone" 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "✓" -ForegroundColor Green
            $createdCount++
        } else {
            Write-Host "✗" -ForegroundColor Red
        }
    } else {
        Write-Host "[DRY-RUN]" -ForegroundColor Yellow
        $createdCount++
    }
}

Write-Host "  Created: $createdCount new milestones`n"

# ============================================================================
# PHASE 2: Assign issues without milestone
# ============================================================================

Write-Host "=== PHASE 2: Assigning unassigned issues ===" -ForegroundColor Green

# Find issues without milestone
Write-Host "  Fetching unassigned issues..." -ForegroundColor Cyan

$unassignedIssues = @()
$page = 1
while ($true) {
    $pageIssues = gh api "repos/$repo/issues?state=all&per_page=100&page=$page" --jq '.[] | select(.milestone == null) | {number: .number, created_at: .created_at}'
    $parsed = @($pageIssues | ConvertFrom-Json -ErrorAction SilentlyContinue)
    
    if ($parsed.Count -eq 0) { break }
    
    $unassignedIssues += $parsed
    Write-Host "    Page $($page): $($parsed.Count) issues" -ForegroundColor Gray
    $page++
}

Write-Host "  Total unassigned: $($unassignedIssues.Count) issues`n"

# Map creation dates to versions (simple heuristic: older → v0.9.0/v1.0.0, newer → current)
# Assume project started around 2023 and we're now in March 2026
$allVersions = @("v0.9.0") + $historicalVersions

$unassignedMigrations = @{}

foreach ($issue in $unassignedIssues) {
    $createdDate = [datetime]$issue.created_at
    $daysAgo = [math]::Floor((Get-Date - $createdDate).TotalDays)
    
    # Simple heuristic: distribute based on age
    # Older issues (>2 years) → v0.9.0, then gradual progression
    if ($daysAgo -gt 365) {
        # Very old → v0.9.0 / v1.0.x range
        $targetIdx = if ($daysAgo -gt 900) { 0 } else { [math]::Min(3, [math]::Floor($daysAgo / 150)) }
    } else {
        # Recent issues → distribute across v1.1.0 - current
        $targetIdx = [math]::Min($allVersions.Count - 1, 4 + [math]::Floor($daysAgo / 50))
    }
    
    $target = $allVersions[$targetIdx]
    $unassignedMigrations[$issue.number] = $target
}

Write-Host "  Planned assignments: $($unassignedMigrations.Count)`n"

# ============================================================================
# PHASE 3: Migrate Q-milestones
# ============================================================================

Write-Host "=== PHASE 3: Migrating Q-milestones ===" -ForegroundColor Green

$qToVersions = @{
    "Q1 2027" = @("v2.1.0", "v2.2.0", "v2.3.0")
    "Q2 2026" = @("v1.5.0", "v1.6.0")
    "Q3 2026" = @("v1.7.0", "v1.8.0")
    "Q4 2026" = @("v1.9.0", "v2.0.0")
}

$qMigrations = @{}

foreach ($q in $qToVersions.Keys) {
    $msNumber = gh api repos/$repo/milestones --jq ".[] | select(.title==\"$q\") | .number" 2>$null
    
    if (-not $msNumber) {
        Write-Host "  $q - NOT FOUND" -ForegroundColor Yellow
        continue
    }
    
    Write-Host "  Processing $q (ID: $msNumber)..." -ForegroundColor Cyan
    
    # Fetch ALL issues with pagination
    $allPages = @()
    $page = 1
    while ($true) {
        $pageJson = gh api "repos/$repo/issues?milestone=$msNumber&state=all&per_page=100&page=$page" --jq '.[].number'
        $pageIssues = @($pageJson | Where-Object { -not [string]::IsNullOrEmpty($_) })
        
        if ($pageIssues.Count -eq 0) { break }
        $allPages += $pageIssues
        $page++
    }
    
    Write-Host "    Found: $($allPages.Count) issues"
    
    $versions = $qToVersions[$q]
    foreach ($issueNum in $allPages) {
        # Heuristic: even distribution across versions
        $idx = [array]::IndexOf($allPages, $issueNum) 
        $ratio = if ($allPages.Count -gt 0) { $idx / $allPages.Count } else { 0 }
        
        if ($versions.Count -eq 2) {
            $target = if ($ratio -lt 0.5) { $versions[0] } else { $versions[1] }
        } elseif ($versions.Count -eq 3) {
            if ($ratio -lt 0.33) { $target = $versions[0] }
            elseif ($ratio -lt 0.66) { $target = $versions[1] }
            else { $target = $versions[2] }
        } else {
            $target = $versions[0]
        }
        
        $qMigrations[$issueNum] = $target
    }
}

Write-Host "  Planned: $($qMigrations.Count) migrations`n"

# ============================================================================
# PHASE 4: Combine all migrations
# ============================================================================

$allMigrations = $unassignedMigrations + $qMigrations
Write-Host "╔════════════════════════════════════════════════════════════╗" -ForegroundColor Blue
Write-Host "║  Summary                                                   ║" -ForegroundColor Blue
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor Blue
Write-Host ""
Write-Host "Historical milestones to create: $createdCount"
Write-Host "Unassigned issues to assign:    $($unassignedMigrations.Count)"
Write-Host "Q-milestones to migrate:       $($qMigrations.Count)"
Write-Host "TOTAL MIGRATIONS:              $($allMigrations.Count)"
Write-Host ""

if ($DryRun) {
    Write-Host "DRY-RUN MODE - Sample migrations:" -ForegroundColor Yellow
    $allMigrations.GetEnumerator() | Select-Object -First 30 | ForEach-Object {
        Write-Host "  #$($_.Name) → $($_.Value)"
    }
    Write-Host "`n  ... and $($allMigrations.Count - 30) more`n" -ForegroundColor Gray
    Write-Host "Run with -DryRun`$false to apply all changes." -ForegroundColor Yellow
    exit 0
}

# ============================================================================
# PHASE 5: Apply all migrations
# ============================================================================

Write-Host "APPLYING ALL MIGRATIONS..." -ForegroundColor Green

$successful = 0
$failed = 0
$idx = 0

foreach ($entry in $allMigrations.GetEnumerator()) {
    $idx++
    $issueNum = $entry.Name
    $targetMs = $entry.Value
    
    Write-Host -NoNewline "[$idx/$($allMigrations.Count)] #$issueNum → $targetMs... "
    
    try {
        $_ = gh issue edit $issueNum --milestone $targetMs -R $repo 2>&1
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

Write-Host "`n✅ Complete: $successful successful, $failed failed" -ForegroundColor Green
