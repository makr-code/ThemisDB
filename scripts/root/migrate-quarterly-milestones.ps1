#!/usr/bin/env pwsh

################################################################################
# ThemisDB: Intelligent Quarterly → Version Milestone Migration
# Purpose: Extract issue mappings from ROADMAP.md files, then intelligently
#          reassign all Q2/Q3/Q4/Q1 milestone issues to version milestones
# Usage: .\scripts\root\migrate-quarterly-milestones.ps1
# Strategy: 
#   1. Parse all src/*/ROADMAP.md for explicit Issue → Version mappings
#   2. Apply those mappings first
#   3. Heuristically assign remaining Q-milestone issues based on issue age
################################################################################

param(
  [string]$Repo = "makr-code/ThemisDB",
  [switch]$DryRun = $false,
  [switch]$Verbose = $false
)

$ErrorActionPreference = "Continue"
$script:mappingRules = @{}
$script:successCount = 0
$script:failCount = 0

################################################################################
# Step 1: Parse ROADMAP.md files for explicit issue-to-version mappings
################################################################################

function Get-RoadmapMappings {
  Write-Host "`n[PARSING] Extracting issue→version mappings from ROADMAP.md files..." -ForegroundColor Blue
  
  $roadmapFiles = Get-ChildItem -Path c:\VCC\themis\src -Filter "ROADMAP.md" -Recurse
  $mappings = @{}
  
  foreach ($file in $roadmapFiles) {
    $module = $file.Directory.Name
    $content = Get-Content $file.FullName -Raw
    
    # Pattern: "Issue: #1234" followed by "(Target: Q1 2026)" or "(Target: v1.5.0)"
    # Also: "(Issue: #1234) (Target: v1.7.0)" or similar
    
    # Extract all Issue references with their targets
    $issuePattern = 'Issue[:\s]+#(\d+).*?(?:\(Target:\s+([vQ]\S+)\)|Target:\s+([vQ]\S+)|milestone[:\s]+([vQ\d.]+))'
    
    $matches = [regex]::Matches($content, $issuePattern, [Text.RegularExpressions.RegexOptions]::IgnoreCase -bor [Text.RegularExpressions.RegexOptions]::Singleline)
    
    foreach ($match in $matches) {
      $issueNum = $match.Groups[1].Value
      $target = $match.Groups[2].Value
      if (-not $target) { $target = $match.Groups[3].Value }
      if (-not $target) { $target = $match.Groups[4].Value }
      
      if ($issueNum -and $target) {
        # Normalize version format (Q2 2026 → map separately)
        if ($target -match '^v\d+\.\d+\.\d+') {
          if (-not $mappings.ContainsKey($issueNum)) {
            $mappings[$issueNum] = $target
            if ($Verbose) {
              Write-Host "  Found: #$issueNum → $target (from $module)" -ForegroundColor Cyan
            }
          }
        }
      }
    }
  }
  
  Write-Host "  Total ROADMAP mappings found: $($mappings.Count)" -ForegroundColor Green
  return $mappings
}

################################################################################
# Step 2: Build quarterly-to-version mapping rules
################################################################################

function Get-QuarterlyVersionMapping {
  # Strategic mapping for unmatched issues
  return @{
    "Q2 2026" = @{ versions = @("v1.5.0", "v1.6.0"); split = 0.5 }
    "Q3 2026" = @{ versions = @("v1.7.0", "v1.8.0"); split = 0.5 }
    "Q4 2026" = @{ versions = @("v1.9.0", "v2.0.0"); split = 0.5 }
    "Q1 2027" = @{ versions = @("v2.1.0", "v2.2.0", "v2.3.0"); split = 0.33 }
  }
}

################################################################################
# Step 3: Fetch all issues with quarterly milestones
################################################################################

function Get-QuarterlyMilestoneIssues {
  param([string]$QuarterlyMilestone)
  
  Write-Host "  Fetching issues for milestone '$QuarterlyMilestone'..." -ForegroundColor Cyan
  
  try {
    # Fetch both open and closed issues with the quarterly milestone
    $issues = gh api repos/$Repo/issues `
      -f state=all `
      --jq ".[] | select(.milestone.title==\"$QuarterlyMilestone\") | {number: .number, created_at: .created_at, title: .title}" `
      -L 300 2>$null | ConvertFrom-Json
    
    return @($issues)
  } catch {
    Write-Host "    ERROR fetching issues: $_" -ForegroundColor Red
    return @()
  }
}

################################################################################
# Step 4: Assign issue to version milestone (with dry-run support)
################################################################################

function Assign-IssueToMilestone {
  param(
    [int]$IssueNumber,
    [string]$VersionMilestone,
    [string]$Reason = ""
  )
  
  Write-Host -NoNewline "    #$IssueNumber → $VersionMilestone "
  if ($Reason) { Write-Host -NoNewline "($Reason) " }
  Write-Host -NoNewline "... "
  
  if ($DryRun) {
    Write-Host "[DRY-RUN]" -ForegroundColor Yellow
    return $true
  }
  
  try {
    $output = gh issue edit $IssueNumber --milestone $VersionMilestone -R $Repo 2>&1
    
    if ($LASTEXITCODE -eq 0) {
      Write-Host "✓" -ForegroundColor Green
      $script:successCount++
      return $true
    } else {
      Write-Host "✗ ($output)" -ForegroundColor Red
      $script:failCount++
      return $false
    }
  } catch {
    Write-Host "✗ (Exception: $_)" -ForegroundColor Red
    $script:failCount++
    return $false
  }
}

################################################################################
# Step 5: Heuristic assignment based on issue age + number
################################################################################

function Get-TargetVersionByHeuristic {
  param(
    [string]$QuarterlyMilestone,
    [int]$IssueNumber,
    [datetime]$CreatedAt,
    [hashtable]$Mappings
  )
  
  # Rule 1: Check if issue is in explicit ROADMAP mapping
  if ($Mappings.ContainsKey($IssueNumber.ToString())) {
    return @{
      version = $Mappings[$IssueNumber.ToString()]
      reason = "ROADMAP"
    }
  }
  
  # Rule 2: Use quarterly-to-version mapping + heuristic split
  $quarterlyMap = Get-QuarterlyVersionMapping
  
  if (-not $quarterlyMap.ContainsKey($QuarterlyMilestone)) {
    return $null
  }
  
  $rule = $quarterlyMap[$QuarterlyMilestone]
  $versions = $rule.versions
  $split = $rule.split
  
  # Heuristic: Use issue creation date or issue number for deterministic split
  # Newer issues → later version
  $daysSinceEpoch = [math]::Floor(($CreatedAt - (Get-Date "2026-01-01")).TotalDays)
  $hashValue = ($IssueNumber + $daysSinceEpoch) % 100
  
  $selectedIndex = if ($hashValue -lt ($split * 100)) { 0 } else { 1 }
  $selectedIndex = [math]::Min($selectedIndex, $versions.Count - 1)
  
  return @{
    version = $versions[$selectedIndex]
    reason = "HEURISTIC[${QuarterlyMilestone}:$hashValue]"
  }
}

################################################################################
# MAIN EXECUTION
################################################################################

Write-Host "`n╔════════════════════════════════════════════════════════════╗" -ForegroundColor Blue
Write-Host "║  ThemisDB: Quarterly→Version Milestone Migration            ║" -ForegroundColor Blue
if ($DryRun) {
  Write-Host "║  Mode: DRY-RUN (no changes will be made)                   ║" -ForegroundColor Yellow
}
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor Blue

# Verify GitHub CLI
Write-Host -NoNewline "`nVerifying GitHub CLI... "
if (-not (gh --version 2>$null)) {
  Write-Host "✗" -ForegroundColor Red
  exit 1
}
Write-Host "✓" -ForegroundColor Green

# Step 1: Get ROADMAP mappings
$roadmapMappings = Get-RoadmapMappings

# Step 2: Get quarterly milestone mapping rules
$quarterlyMap = Get-QuarterlyVersionMapping

# Step 3: Process each quarterly milestone
$quarterlyMilestones = @("Q2 2026", "Q3 2026", "Q4 2026", "Q1 2027")
$totalProcessed = 0

foreach ($quarterly in $quarterlyMilestones) {
  Write-Host "`n[PROCESSING] $quarterly" -ForegroundColor Blue
  
  $issues = Get-QuarterlyMilestoneIssues -QuarterlyMilestone $quarterly
  $count = @($issues).Count
  
  if ($count -eq 0) {
    Write-Host "  (No issues found)" -ForegroundColor Gray
    continue
  }
  
  Write-Host "  Found $count issues, reassigning to versions..." -ForegroundColor Cyan
  
  # Sort by creation date (oldest first) for consistent assignment
  if ($issues -is [array]) {
    $issues = @($issues | Sort-Object -Property created_at)
  }
  
  foreach ($issue in @($issues)) {
    $issueNum = $issue.number
    $createdAt = [datetime]$issue.created_at
    
    $assignment = Get-TargetVersionByHeuristic -QuarterlyMilestone $quarterly `
                                                -IssueNumber $issueNum `
                                                -CreatedAt $createdAt `
                                                -Mappings $roadmapMappings
    
    if ($assignment) {
      $success = Assign-IssueToMilestone -IssueNumber $issueNum `
                                         -VersionMilestone $assignment.version `
                                         -Reason $assignment.reason
      $totalProcessed++
    }
  }
}

################################################################################
# SUMMARY
################################################################################

Write-Host "`n╔════════════════════════════════════════════════════════════╗" -ForegroundColor Blue
Write-Host "║  Migration Summary                                         ║" -ForegroundColor Blue
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor Blue
Write-Host ""
Write-Host "Total Processed:  $totalProcessed"
Write-Host "Successful:       $($script:successCount)" -ForegroundColor Green
Write-Host "Failed:           $($script:failCount)" -ForegroundColor Red
Write-Host ""

if ($DryRun) {
  Write-Host "✓ DRY-RUN complete. No changes made to GitHub." -ForegroundColor Cyan
  Write-Host "  Run without -DryRun to apply changes." -ForegroundColor Cyan
} else {
  if ($script:failCount -eq 0) {
    Write-Host "✓ All issues successfully migrated!" -ForegroundColor Green
  } else {
    Write-Host "⚠ Some issues failed. Check migration errors above." -ForegroundColor Yellow
  }
}

exit ($script:failCount -gt 0 ? 1 : 0)
