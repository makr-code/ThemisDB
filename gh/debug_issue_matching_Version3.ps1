#Requires -Version 5.1
<#
.SYNOPSIS
    Debug Issue Matching - SIMPLIFIED für PowerShell 5.1
#>

param(
    [string]$Repository = "makr-code/ThemisDB",
    [string]$ModuleName = "utils",
    [string]$SourcePath = "..\src"
)

# ============================================================================
# SIMPLE STRING SIMILARITY (ohne mehrdimensionale Arrays)
# ============================================================================

function Get-SimpleSimilarity {
    param(
        [string]$String1,
        [string]$String2
    )
    
    if ($String1 -eq $String2) { return 1.0 }
    if ([string]::IsNullOrWhiteSpace($String1) -or [string]::IsNullOrWhiteSpace($String2)) { return 0.0 }
    
    # Methode 1: Exact Match
    if ($String1.ToLower() -eq $String2.ToLower()) {
        return 1.0
    }
    
    # Methode 2: Enthält-Check (Substring)
    $s1Lower = $String1.ToLower()
    $s2Lower = $String2.ToLower()
    
    if ($s1Lower.Contains($s2Lower) -or $s2Lower.Contains($s1Lower)) {
        $shortLen = [Math]::Min($String1.Length, $String2.Length)
        $longLen = [Math]::Max($String1.Length, $String2.Length)
        return [double]$shortLen / [double]$longLen
    }
    
    # Methode 3: Character-basierte Ähnlichkeit
    $chars1 = $String1.ToLower().ToCharArray() | Sort-Object
    $chars2 = $String2.ToLower().ToCharArray() | Sort-Object
    
    $commonChars = 0
    $i = 0
    $j = 0
    
    while ($i -lt $chars1.Length -and $j -lt $chars2.Length) {
        if ($chars1[$i] -eq $chars2[$j]) {
            $commonChars++
            $i++
            $j++
        } elseif ($chars1[$i] -lt $chars2[$j]) {
            $i++
        } else {
            $j++
        }
    }
    
    $maxChars = [Math]::Max($String1.Length, $String2.Length)
    return [double]$commonChars / [double]$maxChars
}

# ============================================================================
# ROADMAP PARSING
# ============================================================================

function Get-RoadmapContent {
    param([string]$RoadmapPath)
    
    if (-not (Test-Path $RoadmapPath)) {
        Write-Host "  ROADMAP nicht gefunden: $RoadmapPath" -ForegroundColor Red
        return $null
    }
    
    $content = Get-Content $RoadmapPath -Raw -Encoding UTF8
    
    # Parse Phasen
    $phases = @()
    $phasePattern = '##\s+(Phase\s+\d+[^#]*?)(?=##|\z)'
    $phaseMatches = [regex]::Matches($content, $phasePattern, 'Singleline')
    
    foreach ($match in $phaseMatches) {
        $phaseContent = $match.Groups[1].Value
        $phaseTitle = ($phaseContent -split "`n")[0].Trim()
        
        # Finde alle Checkboxen
        $itemPattern = '-\s+\[[ x~]\]\s+(.+?)(?:\s*\(Target:.*?\))?\s*$'
        $items = [regex]::Matches($phaseContent, $itemPattern, 'Multiline') | 
            ForEach-Object { $_.Groups[1].Value.Trim() }
        
        if ($items.Count -gt 0) {
            $phases += @{
                Name = $phaseTitle
                PlannedItems = $items
            }
        }
    }
    
    return @{
        Phases = $phases
    }
}

# ============================================================================
# MAIN DEBUG
# ============================================================================

Clear-Host
Write-Host ""
Write-Host "  =============================================================" -ForegroundColor Cyan
Write-Host "  Issue Matching Debugger (SIMPLE)" -ForegroundColor Cyan
Write-Host "  =============================================================" -ForegroundColor Cyan
Write-Host ""

# 1. Lade existierende Issues
Write-Host "  [1/3] Lade existierende Issues aus GitHub..." -ForegroundColor Yellow

try {
    $issuesJson = gh issue list `
        --repo $Repository `
        --state all `
        --search "[$ModuleName] in:title" `
        --limit 200 `
        --json number,title,state 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  FEHLER: $issuesJson" -ForegroundColor Red
        exit 1
    }
    
    $existingIssues = $issuesJson | ConvertFrom-Json
    
    Write-Host "  Gefunden: $($existingIssues.Count) Issues" -ForegroundColor Green
    Write-Host ""
    
    if ($existingIssues.Count -gt 0) {
        Write-Host "  Existierende Issues:" -ForegroundColor Cyan
        foreach ($issue in $existingIssues) {
            $stateColor = if ($issue.state -eq "open") { "Green" } else { "Gray" }
            Write-Host "    #$($issue.number) [$($issue.state.ToUpper())] $($issue.title)" -ForegroundColor $stateColor
        }
        Write-Host ""
    }
    
} catch {
    Write-Host "  EXCEPTION: $_" -ForegroundColor Red
    exit 1
}

# 2. Lade ROADMAP Items
Write-Host "  [2/3] Lade ROADMAP Items..." -ForegroundColor Yellow

# PowerShell 5.1 kompatibles Join-Path
$modulePath = Join-Path -Path $SourcePath -ChildPath $ModuleName
$roadmapPath = Join-Path -Path $modulePath -ChildPath "ROADMAP.md"

Write-Host "  Suche ROADMAP: $roadmapPath" -ForegroundColor Gray

if (-not (Test-Path $roadmapPath)) {
    Write-Host "  ROADMAP nicht gefunden: $roadmapPath" -ForegroundColor Red
    Write-Host ""
    Write-Host "  Versuche alternative Pfade..." -ForegroundColor Yellow
    
    # Alternative Pfade
    $currentDir = Get-Location
    $alternatives = @(
        (Join-Path -Path $currentDir -ChildPath "src\$ModuleName\ROADMAP.md"),
        (Join-Path -Path $currentDir -ChildPath "..\src\$ModuleName\ROADMAP.md"),
        (Join-Path -Path $currentDir -ChildPath "$ModuleName\ROADMAP.md")
    )
    
    foreach ($alt in $alternatives) {
        Write-Host "  Pruefe: $alt" -ForegroundColor Gray
        if (Test-Path $alt) {
            $roadmapPath = $alt
            Write-Host "  GEFUNDEN!" -ForegroundColor Green
            break
        }
    }
    
    if (-not (Test-Path $roadmapPath)) {
        Write-Host ""
        Write-Host "  Keine ROADMAP gefunden!" -ForegroundColor Red
        Write-Host "  Bitte geben Sie den korrekten Pfad an:" -ForegroundColor Yellow
        Write-Host "  .\debug_issue_matching_SIMPLE.ps1 -SourcePath 'C:\VCC\themis\src' -ModuleName 'utils'" -ForegroundColor White
        exit 1
    }
}

$roadmapData = Get-RoadmapContent -RoadmapPath $roadmapPath

if ($null -eq $roadmapData) {
    Write-Host "  ROADMAP konnte nicht geparst werden!" -ForegroundColor Red
    exit 1
}

$allItems = @()
foreach ($phase in $roadmapData.Phases) {
    foreach ($item in $phase.PlannedItems) {
        $allItems += $item
    }
}

Write-Host "  Gefunden: $($allItems.Count) ROADMAP Items" -ForegroundColor Green
Write-Host ""

if ($allItems.Count -eq 0) {
    Write-Host "  KEINE ROADMAP Items gefunden!" -ForegroundColor Red
    Write-Host "  ROADMAP Inhalt pruefen:" -ForegroundColor Yellow
    Write-Host ""
    Get-Content $roadmapPath -TotalCount 20 | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
    exit 1
}

# 3. Vergleiche jeden ROADMAP Item mit jedem Issue
Write-Host "  [3/3] Vergleiche Items..." -ForegroundColor Yellow
Write-Host ""
Write-Host "  =============================================================" -ForegroundColor Gray

$matchCount = 0
$noMatchCount = 0

foreach ($item in $allItems) {
    Write-Host ""
    Write-Host "  ROADMAP Item: $item" -ForegroundColor White
    Write-Host "  " + ("-" * 60) -ForegroundColor Gray
    
    # Normalisiere ROADMAP Item
    $normalizedItem = $item.ToLower() -replace '\s+', ' ' -replace '[^\w\s]', ''
    Write-Host "  Normalized:   $normalizedItem" -ForegroundColor Gray
    
    $bestMatch = $null
    $bestSimilarity = 0.0
    
    foreach ($issue in $existingIssues) {
        # Entferne Modul-Präfix aus Issue-Titel
        $issueTitle = $issue.title -replace '^\[.+?\]\s*', ''
        $normalizedIssue = $issueTitle.ToLower() -replace '\s+', ' ' -replace '[^\w\s]', ''
        
        $similarity = Get-SimpleSimilarity -String1 $normalizedItem -String2 $normalizedIssue
        
        if ($similarity -gt $bestSimilarity) {
            $bestSimilarity = $similarity
            $bestMatch = $issue
        }
        
        # Debug alle Matches > 30%
        if ($similarity -gt 0.3) {
            $simPercent = [math]::Round($similarity * 100, 1)
            Write-Host "    -> Issue #$($issue.number): $simPercent% match" -ForegroundColor DarkGray
        }
    }
    
    Write-Host ""
    $simPercent = [math]::Round($bestSimilarity * 100, 1)
    
    if ($bestMatch -and $bestSimilarity -gt 0.85) {
        Write-Host "  [MATCH] Issue #$($bestMatch.number)" -ForegroundColor Green
        Write-Host "    Issue Title:  $($bestMatch.title -replace '^\[.+?\]\s*', '')" -ForegroundColor Green
        Write-Host "    Similarity:   $simPercent%" -ForegroundColor Green
        Write-Host "    Status:       $($bestMatch.state.ToUpper())" -ForegroundColor $(if ($bestMatch.state -eq 'open') { 'Green' } else { 'Gray' })
        $matchCount++
    } elseif ($bestMatch -and $bestSimilarity -gt 0.70) {
        Write-Host "  [PARTIAL] Issue #$($bestMatch.number)" -ForegroundColor Yellow
        Write-Host "    Issue Title:   $($bestMatch.title -replace '^\[.+?\]\s*', '')" -ForegroundColor Yellow
        Write-Host "    Similarity:    $simPercent% (need >85%)" -ForegroundColor Yellow
        Write-Host "    Status:        $($bestMatch.state.ToUpper())" -ForegroundColor $(if ($bestMatch.state -eq 'open') { 'Green' } else { 'Gray' })
        Write-Host "    -> Wird NICHT als Match gezaehlt" -ForegroundColor Yellow
        $noMatchCount++
    } elseif ($bestMatch) {
        Write-Host "  [NO MATCH] Zu geringe Aehnlichkeit" -ForegroundColor Red
        Write-Host "    Best Match:    Issue #$($bestMatch.number) ($simPercent%)" -ForegroundColor Red
        $noMatchCount++
    } else {
        Write-Host "  [NO MATCH] Kein Issue gefunden" -ForegroundColor Red
        $noMatchCount++
    }
}

Write-Host ""
Write-Host "  =============================================================" -ForegroundColor Gray
Write-Host ""
Write-Host "  ZUSAMMENFASSUNG:" -ForegroundColor Cyan
Write-Host "  - ROADMAP Items:        $($allItems.Count)" -ForegroundColor White
Write-Host "  - Existierende Issues:  $($existingIssues.Count)" -ForegroundColor White

$openCount = ($existingIssues | Where-Object { $_.state -eq 'open' }).Count
$closedCount = ($existingIssues | Where-Object { $_.state -eq 'closed' }).Count

Write-Host "    - Davon OPEN:         $openCount" -ForegroundColor Green
Write-Host "    - Davon CLOSED:       $closedCount" -ForegroundColor Gray
Write-Host "  - Matches gefunden:     $matchCount" -ForegroundColor $(if ($matchCount -gt 0) { 'Green' } else { 'White' })
Write-Host "  - Keine Matches:        $noMatchCount" -ForegroundColor $(if ($noMatchCount -gt 0) { 'Red' } else { 'White' })
Write-Host ""

if ($matchCount -eq $allItems.Count -and $allItems.Count -gt 0) {
    Write-Host "  [PROBLEM] Alle Items werden als existierend erkannt!" -ForegroundColor Red
    Write-Host "  Dies ist ein FALSE POSITIVE Problem." -ForegroundColor Red
} elseif ($noMatchCount -gt 0) {
    Write-Host "  [OK] Es gibt $noMatchCount Items die noch keine Issues haben." -ForegroundColor Green
    Write-Host "  -> Das Skript sollte $noMatchCount neue Issues erstellen." -ForegroundColor Green
}

# 4. Zeige was erstellt werden würde
if ($noMatchCount -gt 0) {
    Write-Host ""
    Write-Host "  NEUE ISSUES DIE ERSTELLT WERDEN:" -ForegroundColor Cyan
    Write-Host "  =============================================================" -ForegroundColor Gray
    
    $counter = 0
    foreach ($item in $allItems) {
        $normalizedItem = $item.ToLower() -replace '\s+', ' ' -replace '[^\w\s]', ''
        
        $bestSimilarity = 0.0
        foreach ($issue in $existingIssues) {
            $issueTitle = $issue.title -replace '^\[.+?\]\s*', ''
            $normalizedIssue = $issueTitle.ToLower() -replace '\s+', ' ' -replace '[^\w\s]', ''
            $similarity = Get-SimpleSimilarity -String1 $normalizedItem -String2 $normalizedIssue
            if ($similarity -gt $bestSimilarity) {
                $bestSimilarity = $similarity
            }
        }
        
        if ($bestSimilarity -lt 0.85) {
            $counter++
            Write-Host "  $counter. [$ModuleName] $item" -ForegroundColor White
        }
    }
}

Write-Host ""
Write-Host "  Druecken Sie Enter um zu beenden..." -ForegroundColor Gray
$null = Read-Host