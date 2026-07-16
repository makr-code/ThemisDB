#!/usr/bin/env pwsh

################################################################################
# ThemisDB: Intelligent Q-Milestone → Version-Milestone Migration
# Purpose: Remap all Q-milestone issues to version milestones using:
#   1. ROADMAP.md analysis for explicit Issue→Version mappings
#   2. Heuristic distribution for remaining issues
# Usage: .\scripts\root\remigrate-q-milestones.ps1 [-DryRun] [-Verbose]
# Author: Copilot
################################################################################

param(
    [switch]$DryRun = $false,
    [switch]$Verbose = $false
)

$repo = "makr-code/ThemisDB"
$scriptStart = Get-Date

# ============================================================================
# STEP 1: Extract Issue→Version mappings from src/*/ROADMAP.md
# ============================================================================

Write-Host "`n=== STEP 1: Parse ROADMAP.md files ===" -ForegroundColor Cyan

$roadmapMappings = @{}  # Issue# → Version
$roadmapFiles = Get-ChildItem -Path "src" -Recurse -Filter "ROADMAP.md" | Where-Object { $_.FullName -like "*src\*" }

Write-Host "Found $($roadmapFiles.Count) ROADMAP.md files"

foreach ($roadmapFile in $roadmapFiles) {
    if ($Verbose) { Write-Host "  Processing: $($roadmapFile.FullName)" }
    
    $content = Get-Content $roadmapFile.FullName -Raw
    
    # Pattern: (Issue: #XXXX) combined with Target: v1.5.0 or v1.6.0 etc.
    # Also matches: (Target: Q2 2026)
    $issuePattern = '\(Issue:?\s*#(\d+)\)'
    $targetPattern = '\(Target:\s*(v[\d.]+|Q[1-4]\s+\d{4})\)'
    
    # Find all lines with Issue references
    $lines = $content -split "`n"
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        
        if ($line -match $issuePattern) {
            $issueNum = $matches[1]
            
            # Look for Target in same line or nearby lines
            $target = $null
            if ($line -match $targetPattern) {
                $target = $matches[1]
            } else {
                # Check next 2 lines for Target
                for ($j = $i + 1; $j -le [Math]::Min($i + 2, $lines.Count - 1); $j++) {
                    if ($lines[$j] -match $targetPattern) {
                        $target = $matches[1]
                        break
                    }
                }
            }
            
            # Only store if target is a version (v1.x.x, v2.x.x)
            if ($target -and $target -match '^v[12]\.\d+\.\d+$') {
                if (-not $roadmapMappings.ContainsKey($issueNum)) {
                    $roadmapMappings[$issueNum] = $target
                    if ($Verbose) { Write-Host "    #$issueNum → $target" }
                }
            }
        }
    }
}

Write-Host "Extracted $($roadmapMappings.Count) Issue→Version mappings from ROADMAPs`n" -ForegroundColor Green

# ============================================================================
# STEP 2: Fetch all issues from Q-milestones (OPEN + CLOSED)
# ============================================================================

Write-Host "=== STEP 2: Fetch Q-milestone issues ===" -ForegroundColor Cyan

$qMilestones = @("Q1 2027", "Q2 2026", "Q3 2026", "Q4 2026")
$qIssues = @{}  # Quarter → List of issue numbers

foreach ($qMilestone in $qMilestones) {
    # First, get milestone ID by title
    $msNumber = gh api repos/$repo/milestones --jq ".[] | select(.title==\"$qMilestone\") | .number" 2>$null
    
    if (-not $msNumber) {
        Write-Host "$qMilestone - NOT FOUND (0 issues)"
        continue
    }
    
    # Fetch OPEN issues with this milestone ID
    $openIssues = @(gh api repos/$repo/issues `
        -f state="open" `
        -f milestone=$msNumber `
        --jq ".[].number" 2>$null)
    
    # Fetch CLOSED issues with this milestone ID  
    $closedIssues = @(gh api repos/$repo/issues `
        -f state="closed" `
        -f milestone=$msNumber `
        --jq ".[].number" 2>$null)
    
    $allIssues = @($openIssues) + @($closedIssues) | Where-Object { $_ } | Select-Object -Unique
    $qIssues[$qMilestone] = @($allIssues)
    
    Write-Host "$qMilestone - $($allIssues.Count) issues (open: $(($openIssues | Measure-Object).Count), closed: $(($closedIssues | Measure-Object).Count))"
}

$totalQIssues = $qIssues.Values | Measure-Object -Sum -Property Count | Select-Object -ExpandProperty Sum
Write-Host "Total Q-milestone issues: $totalQIssues`n" -ForegroundColor Green

# ============================================================================
# STEP 3: Build intelligent mapping for each Q→Version(s)
# ============================================================================

Write-Host "=== STEP 3: Build Q→Version mappings ===" -ForegroundColor Cyan

$migrations = @{}  # Issue# → TargetVersion

# Mapping Rules:
#   Q2 2026 → v1.5.0 (earlier) + v1.6.0 (later)
#   Q3 2026 → v1.7.0 + v1.8.0 (split 50/50)
#   Q4 2026 → v1.9.0 + v2.0.0 (split 50/50)
#   Q1 2027 → v2.1.0 + v2.2.0 + v2.3.0 (split even)

$qVersionMaps = @{
    "Q2 2026" = @("v1.5.0", "v1.6.0")
    "Q3 2026" = @("v1.7.0", "v1.8.0")
    "Q4 2026" = @("v1.9.0", "v2.0.0")
    "Q1 2027" = @("v2.1.0", "v2.2.0", "v2.3.0")
}

foreach ($qMilestone in $qMilestones) {
    $issues = $qIssues[$qMilestone]
    $versions = $qVersionMaps[$qMilestone]
    
    Write-Host "Processing $qMilestone → [$($versions -join ', ')]"
    Write-Host "  Issues: $($issues.Count)"
    Write-Host "  Using distribution: " -NoNewline
    
    $processedViaRoadmap = 0
    $processedViaHeuristic = 0
    
    for ($idx = 0; $idx -lt $issues.Count; $idx++) {
        $issueNum = $issues[$idx]
        
        # First: Check if issue is in ROADMAP mappings
        if ($roadmapMappings.ContainsKey($issueNum)) {
            $targetVersion = $roadmapMappings[$issueNum]
            $migrations[$issueNum] = $targetVersion
            $processedViaRoadmap++
        } else {
            # Second: Use heuristic - distribute based on issue index
            $ratio = $idx / $issues.Count
            
            if ($versions.Count -eq 2) {
                # 50/50 split
                $targetVersion = if ($ratio -lt 0.5) { $versions[0] } else { $versions[1] }
            } elseif ($versions.Count -eq 3) {
                # Thirds
                if ($ratio -lt 0.33) { $targetVersion = $versions[0] }
                elseif ($ratio -lt 0.66) { $targetVersion = $versions[1] }
                else { $targetVersion = $versions[2] }
            } else {
                $targetVersion = $versions[0]
            }
            
            $migrations[$issueNum] = $targetVersion
            $processedViaHeuristic++
        }
    }
    
    Write-Host "ROADMAP: $processedViaRoadmap, Heuristic: $processedViaHeuristic`n"
}

Write-Host "Total migrations planned: $($migrations.Count)`n" -ForegroundColor Green

# ============================================================================
# STEP 4: Display mapping (Dry-Run or Summary)
# ============================================================================

if ($DryRun) {
    Write-Host "=== DRY-RUN: Sample of planned migrations ===" -ForegroundColor Yellow
    
    $sortedMigrations = $migrations.GetEnumerator() | Sort-Object -Property Name | Select-Object -First 50
    
    foreach ($entry in $sortedMigrations) {
        Write-Host "  #$($entry.Name) → $($entry.Value)"
    }
    
    if ($migrations.Count -gt 50) {
        Write-Host "  ... and $($migrations.Count - 50) more issues`n"
    }
    
    Write-Host "DRY-RUN COMPLETE. Re-run with -DryRun `$false to apply changes." -ForegroundColor Yellow
    exit 0
}

# ============================================================================
# STEP 5: Apply migrations
# ============================================================================

Write-Host "=== STEP 5: Apply migrations ===" -ForegroundColor Cyan

$successful = 0
$failed = 0
$idx = 0

foreach ($entry in $migrations.GetEnumerator()) {
    $issueNum = $entry.Name
    $targetVersion = $entry.Value
    $idx++
    
    Write-Host -NoNewline "[$idx/$($migrations.Count)] #$issueNum → $targetVersion ... "
    
    try {
        $output = gh issue edit $issueNum --milestone $targetVersion -R $repo 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "✓" -ForegroundColor Green
            $successful++
        } else {
            Write-Host "✗ (exit code: $LASTEXITCODE)" -ForegroundColor Red
            $failed++
        }
    } catch {
        Write-Host "✗ (exception)" -ForegroundColor Red
        $failed++
    }
    
    # Rate limit: 1 request per 100ms
    Start-Sleep -Milliseconds 100
}

# ============================================================================
# SUMMARY
# ============================================================================

$scriptEnd = Get-Date
$duration = ($scriptEnd - $scriptStart).TotalSeconds

Write-Host "`n╔════════════════════════════════════════════════════════════╗" -ForegroundColor Blue
Write-Host "║  Q-Milestone Remigration Summary                            ║" -ForegroundColor Blue
Write-Host "╚════════════════════════════════════════════════════════════╝" -ForegroundColor Blue
Write-Host ""
Write-Host "ROADMAP-guided mappings:   $($roadmapMappings.Count)"
Write-Host "Total migrations applied:  $($migrations.Count)"
Write-Host "Successful:                $successful" -ForegroundColor Green
Write-Host "Failed:                    $failed" -ForegroundColor $(if ($failed -eq 0) { "Green" } else { "Red" })
Write-Host "Duration:                  $([math]::Round($duration, 1))s`n"

if ($failed -eq 0) {
    Write-Host "✓ All Q-milestone issues successfully remigrated!" -ForegroundColor Green
} else {
    Write-Host "⚠ $failed issues failed. Review and retry." -ForegroundColor Yellow
}
