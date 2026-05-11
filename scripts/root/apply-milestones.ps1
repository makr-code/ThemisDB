#!/usr/bin/env pwsh

################################################################################
# ThemisDB: Batch Milestone Application Script (PowerShell)
# Purpose: Assign milestones to all GitHub issues
# Usage: .\scripts\root\apply-milestones.ps1
# Requirements: GitHub CLI (gh) installed and authenticated
# Status: Using REAL issue numbers from makr-code/ThemisDB
################################################################################

$repo = "makr-code/ThemisDB"
$scriptStart = Get-Date

# Issue-Milestone mapping
$issues = @(
  @{num=3755; ms="v2.0.0"}
  @{num=3754; ms="v1.9.0"}
  @{num=3753; ms="v2.1.0"}
  @{num=3752; ms="v2.1.0"}
  @{num=3751; ms="v2.0.0"}
  @{num=3750; ms="v2.0.0"}
  @{num=3749; ms="v1.8.0"}
  @{num=3747; ms="v1.6.0"}
  @{num=3745; ms="v2.0.0"}
  @{num=3744; ms="v2.0.0"}
  @{num=3743; ms="v2.0.0"}
  @{num=3742; ms="v2.0.0"}
  @{num=3741; ms="v2.0.0"}
  @{num=3740; ms="v1.8.0"}
  @{num=3735; ms="v1.7.0"}
  @{num=3734; ms="v2.3.0"}
  @{num=3731; ms="v1.7.0"}
  @{num=3730; ms="v1.8.0"}
  @{num=3729; ms="v1.7.0"}
  @{num=3728; ms="v1.6.0"}
  @{num=3726; ms="v1.7.0"}
  @{num=3724; ms="v1.7.0"}
  @{num=3723; ms="v1.8.0"}
  @{num=3716; ms="v1.5.0"}
  @{num=3715; ms="v1.5.0"}
  @{num=3713; ms="v1.5.0"}
  @{num=3712; ms="v1.5.0"}
  @{num=3711; ms="v1.5.0"}
  @{num=3709; ms="v1.5.0"}
  @{num=3708; ms="v1.5.0"}
)

Write-Host "`n╔════════════════════════════════════════════════════════════╗" -ForegroundColor Blue
Write-Host "║  ThemisDB: Batch Milestone Application (PowerShell)         ║" -ForegroundColor Blue
Write-Host "║  Processing $($issues.Count) Real Issues from Repository                   ║" -ForegroundColor Blue
Write-Host "╚════════════════════════════════════════════════════════════╝`n" -ForegroundColor Blue

# Check GitHub CLI availability
Write-Host -NoNewline "Checking GitHub CLI... "
try {
  $ghVersion = gh --version 2>$null
  Write-Host "✓" -ForegroundColor Green
} catch {
  Write-Host "✗ (GitHub CLI not found)" -ForegroundColor Red
  exit 1
}

# Verify authentication
Write-Host -NoNewline "Verifying authentication... "
try {
  $authStatus = gh auth status -R $repo 2>$null
  Write-Host "✓" -ForegroundColor Green
} catch {
  Write-Host "✗ (Not authenticated)" -ForegroundColor Red
  exit 1
}

Write-Host ""

# Counter variables
$processed = 0
$successful = 0
$failed = 0

# Process all issues
foreach ($issue in $issues) {
  $processed++
  $num = $issue.num
  $ms = $issue.ms
  
  Write-Host -NoNewline "[$processed/$($issues.Count)] [MILESTONE] Setting issue #$num to $ms... "
  
  try {
    # Update issue with milestone
    $output = gh issue edit $num --milestone $ms -R $repo 2>&1
    
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
}

################################################################################
# SUMMARY REPORT
################################################################################

$scriptEnd = Get-Date
$duration = ($scriptEnd - $scriptStart).TotalSeconds

Write-Host ""
Write-Host "╔════════════════════════════════════════════════════════════╗" -ForegroundColor Blue
Write-Host "║  Milestone Application Summary                             ║" -ForegroundColor Blue
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor Blue
Write-Host ""
Write-Host "Total Issues Processed: $processed"
Write-Host "Successful:            $successful" -ForegroundColor Green
Write-Host "Failed:                $failed" -ForegroundColor Red
Write-Host "Duration:              $([math]::Round($duration, 1))s`n" -ForegroundColor Blue

if ($failed -eq 0) {
  Write-Host "✓ All 30 milestones successfully assigned!" -ForegroundColor Green
  exit 0
} else {
  Write-Host "⚠ Some milestones failed to apply." -ForegroundColor Yellow
  exit 1
}
