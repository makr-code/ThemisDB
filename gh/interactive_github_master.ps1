#Requires -Version 5.1
<#
.SYNOPSIS
    GitHub Issue Management System - Ollama AI Edition (COMPLETE & FIXED)
.DESCRIPTION
    Vollständiges interaktives System zur Verwaltung von GitHub Issues aus ROADMAPs
.VERSION
    6.0.0 - Complete Rewrite
#>

param(
    [string]$Repository = "makr-code/ThemisDB",
    [string]$SourcePath = $null,
    [string]$OllamaUrl = "http://localhost:11434",
    [string]$OllamaModel = "llama3.2",
    [switch]$DryRun = $false,
    [switch]$AutoCommit = $false,
    [switch]$UseAI = $true
)

# ============================================================================
# GLOBALE VARIABLEN
# ============================================================================

$script:RepoRoot = $null
$script:SourceDir = $null
$script:UseAI = $UseAI
$script:OllamaUrl = $OllamaUrl
$script:OllamaModel = $OllamaModel

# ============================================================================
# UI FUNCTIONS
# ============================================================================

function Show-Banner {
    param([string]$Title = "GitHub Issue Manager - AI Edition")
    
    Clear-Host
    Write-Host ""
    Write-Host "  =============================================================" -ForegroundColor Cyan
    Write-Host "  $Title" -ForegroundColor Cyan
    Write-Host "  =============================================================" -ForegroundColor Cyan
    Write-Host ""
}

function Wait-Continue {
    param([string]$Message = "Druecken Sie Enter um fortzufahren...")
    
    Write-Host ""
    Write-Host "  $Message" -ForegroundColor Gray
    $null = Read-Host
}

function Get-UserChoice {
    param(
        [string]$Prompt = "Ihre Wahl",
        [array]$Options,
        [int]$Default = 1
    )
    
    Write-Host ""
    Write-Host "  $Prompt" -ForegroundColor Yellow
    Write-Host ""
    
    for ($i = 0; $i -lt $Options.Count; $i++) {
        $marker = if (($i + 1) -eq $Default) { ">" } else { " " }
        Write-Host "    $marker [$($i + 1)] $($Options[$i])" -ForegroundColor White
    }
    
    Write-Host ""
    Write-Host "  Ihre Wahl [1-$($Options.Count)]: " -NoNewline -ForegroundColor Yellow
    $input = Read-Host
    
    if ([string]::IsNullOrWhiteSpace($input)) {
        return $Default
    }
    
    $choice = 0
    if ([int]::TryParse($input, [ref]$choice) -and $choice -ge 1 -and $choice -le $Options.Count) {
        return $choice
    }
    
    return $Default
}

# ============================================================================
# STRING SIMILARITY
# ============================================================================

function Get-SimpleSimilarity {
    param([string]$String1, [string]$String2)
    
    if ($String1 -eq $String2) { return 1.0 }
    if ([string]::IsNullOrWhiteSpace($String1) -or [string]::IsNullOrWhiteSpace($String2)) { return 0.0 }
    
    if ($String1.ToLower() -eq $String2.ToLower()) { return 1.0 }
    
    $s1Lower = $String1.ToLower()
    $s2Lower = $String2.ToLower()
    
    if ($s1Lower.Contains($s2Lower) -or $s2Lower.Contains($s1Lower)) {
        $shortLen = [Math]::Min($String1.Length, $String2.Length)
        $longLen = [Math]::Max($String1.Length, $String2.Length)
        return [double]$shortLen / [double]$longLen
    }
    
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
# GITHUB CLI FUNCTIONS
# ============================================================================

function Test-GitHubCLI {
    try {
        $null = gh --version 2>&1
        return $LASTEXITCODE -eq 0
    } catch {
        return $false
    }
}

function Test-GitHubAuth {
    try {
        $result = gh auth status 2>&1
        return $LASTEXITCODE -eq 0
    } catch {
        return $false
    }
}

function Get-ExistingIssuesForModule {
    param([string]$Repo, [string]$ModuleName)
    
    Write-Host "  Pruefe vorhandene Issues fuer Modul '$ModuleName'..." -NoNewline
    
    try {
        $issuesJson = gh issue list `
            --repo $Repo `
            --state all `
            --search "[$ModuleName] in:title" `
            --limit 200 `
            --json number,title,state,labels 2>&1
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host " FEHLER" -ForegroundColor Red
            Write-Host "    $issuesJson" -ForegroundColor Red
            return @()
        }
        
        $issues = $issuesJson | ConvertFrom-Json
        
        Write-Host " $($issues.Count) gefunden" -ForegroundColor Green
        
        return $issues
        
    } catch {
        Write-Host " EXCEPTION" -ForegroundColor Red
        Write-Host "    $_" -ForegroundColor Red
        return @()
    }
}

function Test-IssueExists {
    param(
        [string]$ModuleName,
        [string]$ItemTitle,
        [array]$ExistingIssues
    )
    
    $normalizedSearch = $ItemTitle.ToLower() -replace '\s+', ' ' -replace '[^\w\s]', ''
    
    foreach ($issue in $ExistingIssues) {
        $issueTitle = $issue.title -replace '^\[.+?\]\s*', ''
        $normalizedIssue = $issueTitle.ToLower() -replace '\s+', ' ' -replace '[^\w\s]', ''
        
        $similarity = Get-SimpleSimilarity -String1 $normalizedSearch -String2 $normalizedIssue
        
        if ($similarity -gt 0.85) {
            return @{
                Exists = $true
                Issue = $issue
                Similarity = $similarity
            }
        }
    }
    
    return @{
        Exists = $false
        Issue = $null
        Similarity = 0
    }
}

function New-GitHubIssue {
    param(
        [string]$Repo,
        [string]$Title,
        [string]$Body,
        [array]$Labels = @()
    )
    
    try {
        # Escape für JSON
        $titleEscaped = $Title -replace '\\', '\\\\' -replace '"', '\"' -replace "`n", ' ' -replace "`r", ''
        $bodyEscaped = $Body -replace '\\', '\\\\' -replace '"', '\"' -replace "`n", '\n' -replace "`r", ''
        
        # Labels array als JSON
        $labelsJson = ($Labels | ForEach-Object { "`"$_`"" }) -join ","
        
        $json = @"
{
  "title": "$titleEscaped",
  "body": "$bodyEscaped",
  "labels": [$labelsJson]
}
"@
        
        Write-Host "    Erstelle Issue..." -NoNewline -ForegroundColor Cyan
        
        $result = $json | gh api "repos/$Repo/issues" -X POST --input - 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host " ERFOLG!" -ForegroundColor Green
            
            try {
                $issueData = $result | ConvertFrom-Json
                Write-Host "    Issue #$($issueData.number) erstellt: $($issueData.html_url)" -ForegroundColor Gray
            } catch {
                Write-Host "    Issue erstellt" -ForegroundColor Gray
            }
            
            return $true
        } else {
            Write-Host " FEHLER!" -ForegroundColor Red
            Write-Host "    GitHub API Error: $result" -ForegroundColor Red
            return $false
        }
        
    } catch {
        Write-Host " EXCEPTION!" -ForegroundColor Red
        Write-Host "    Error: $_" -ForegroundColor Red
        return $false
    }
}

# ============================================================================
# REPOSITORY FUNCTIONS
# ============================================================================

function Find-RepositoryRoot {
    $current = Get-Location
    
    while ($current) {
        if (Test-Path (Join-Path -Path $current -ChildPath ".git")) {
            return $current.Path
        }
        $current = $current.Parent
    }
    
    return (Get-Location).Path
}

function Find-SourceDirectory {
    param([string]$RepoRoot)
    
    $candidates = @("src", "source", "lib", "packages", "modules")
    
    foreach ($candidate in $candidates) {
        $path = Join-Path -Path $RepoRoot -ChildPath $candidate
        if (Test-Path $path) {
            return $path
        }
    }
    
    return $null
}

# ============================================================================
# ROADMAP PARSING (UNIVERSAL)
# ============================================================================

function Get-RoadmapContent {
    param([string]$RoadmapPath)
    
    if (-not (Test-Path $RoadmapPath)) {
        Write-Host "  ROADMAP nicht gefunden: $RoadmapPath" -ForegroundColor Red
        return $null
    }
    
    $content = Get-Content $RoadmapPath -Raw -Encoding UTF8
    
    # Parse ALLE Checkboxen (unabhängig von Überschriften)
    $openItems = @()
    $completedItems = @()
    $inProgressItems = @()
    
    $checkboxPattern = '-\s+\[([x ~])\]\s+(.+?)(?:\s*\(Target:.*?\))?\s*$'
    $matches = [regex]::Matches($content, $checkboxPattern, 'Multiline')
    
    foreach ($match in $matches) {
        $status = $match.Groups[1].Value.Trim()
        $itemText = $match.Groups[2].Value.Trim()
        
        switch ($status) {
            ' ' { $openItems += $itemText }
            'x' { $completedItems += $itemText }
            '~' { $inProgressItems += $itemText }
        }
    }
    
    return @{
        OpenItems = $openItems
        InProgressItems = $inProgressItems
        CompletedItems = $completedItems
        AllItems = $openItems + $inProgressItems
    }
}

# ============================================================================
# MODULE FUNCTIONS
# ============================================================================

function Select-Module {
    param([string]$Purpose = "Aktion")
    
    Show-Banner "Modul auswaehlen fuer: $Purpose"
    
    $modules = Get-ChildItem $script:SourceDir -Directory | Where-Object {
        Test-Path (Join-Path -Path $_.FullName -ChildPath "ROADMAP.md")
    } | Sort-Object Name
    
    if ($modules.Count -eq 0) {
        Write-Host "  Keine Module mit ROADMAP gefunden!" -ForegroundColor Red
        Wait-Continue
        return $null
    }
    
    Write-Host "  Verfuegbare Module ($($modules.Count)):" -ForegroundColor Cyan
    Write-Host ""
    
    for ($i = 0; $i -lt $modules.Count; $i++) {
        Write-Host "    [$($i + 1)] $($modules[$i].Name)" -ForegroundColor White
    }
    
    Write-Host "    [0] Abbrechen" -ForegroundColor Gray
    Write-Host ""
    
    Write-Host "  Ihre Wahl [0-$($modules.Count)]: " -NoNewline -ForegroundColor Yellow
    $choice = Read-Host
    
    $index = 0
    if ([int]::TryParse($choice, [ref]$index) -and $index -ge 1 -and $index -le $modules.Count) {
        $selected = $modules[$index - 1]
        return @{
            Name = $selected.Name
            Path = $selected.FullName
            RoadmapPath = Join-Path -Path $selected.FullName -ChildPath "ROADMAP.md"
        }
    }
    
    return $null
}

# ============================================================================
# OLLAMA API INTEGRATION
# ============================================================================

function Test-OllamaAvailable {
    try {
        $response = Invoke-RestMethod -Uri "$script:OllamaUrl/api/tags" -Method Get -TimeoutSec 3 -ErrorAction Stop
        return $true
    } catch {
        return $false
    }
}

function Invoke-OllamaChat {
    param(
        [string]$SystemPrompt,
        [string]$UserPrompt,
        [string]$Model = $script:OllamaModel
    )
    
    try {
        $body = @{
            model = $Model
            messages = @(
                @{ role = "system"; content = $SystemPrompt },
                @{ role = "user"; content = $UserPrompt }
            )
            stream = $false
            options = @{ temperature = 0.7; num_predict = 500 }
        } | ConvertTo-Json -Depth 10
        
        $response = Invoke-RestMethod `
            -Uri "$script:OllamaUrl/api/chat" `
            -Method Post `
            -Body $body `
            -ContentType "application/json" `
            -TimeoutSec 60 `
            -ErrorAction Stop
        
        return $response.message.content
    } catch {
        Write-Host "    Ollama Chat API Fehler: $_" -ForegroundColor Red
        return $null
    }
}

function Optimize-IssueTitle {
    param(
        [string]$OriginalTitle,
        [string]$ModuleName
    )
    
    $systemPrompt = @"
You are a GitHub issue title optimizer. Create concise, clear, actionable titles.
Rules: Use imperative mood, be specific, keep under 80 chars, include key technical terms.
"@
    
    $userPrompt = @"
Module: $ModuleName
Original title: $OriginalTitle

Optimize this GitHub issue title. Return ONLY the optimized title, nothing else.
"@
    
    Write-Host "    AI optimiert Titel..." -NoNewline -ForegroundColor Cyan
    
    $optimized = Invoke-OllamaChat -SystemPrompt $systemPrompt -UserPrompt $userPrompt
    
    if ($optimized) {
        $optimized = $optimized.Trim().Trim('"').Trim("'")
        Write-Host " OK" -ForegroundColor Green
        return $optimized
    }
    
    Write-Host " FEHLER (Original verwendet)" -ForegroundColor Yellow
    return $OriginalTitle
}

# ============================================================================
# AI-ENHANCED ISSUE CREATION
# ============================================================================

function New-ModuleIssuesAI {
    param(
        [string]$ModuleName,
        [hashtable]$RoadmapData,
        [switch]$UseAI,
        [switch]$DryRun
    )
    
    Show-Banner "AI-Enhanced Issue-Erstellung: $ModuleName"
    
    # Prüfe Ollama
    Write-Host "  Pruefe Ollama..." -NoNewline
    $ollamaAvailable = Test-OllamaAvailable
    
    if (-not $ollamaAvailable -or -not $UseAI) {
        Write-Host " NICHT VERFUEGBAR" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "  AI-Features werden deaktiviert" -ForegroundColor Yellow
        $UseAI = $false
    } else {
        Write-Host " OK (Modell: $script:OllamaModel)" -ForegroundColor Green
    }
    
    Write-Host ""
    
    # Lade vorhandene Issues
    $existingIssues = Get-ExistingIssuesForModule -Repo $Repository -ModuleName $ModuleName
    Write-Host ""
    
    # Items sammeln die erstellt werden müssen
    $itemsToCreate = @()
    
    foreach ($item in $RoadmapData.AllItems) {
        $check = Test-IssueExists -ModuleName $ModuleName -ItemTitle $item -ExistingIssues $existingIssues
        
        if (-not $check.Exists) {
            $itemsToCreate += $item
        }
    }
    
    if ($itemsToCreate.Count -eq 0) {
        Write-Host "  Keine neuen Issues zu erstellen!" -ForegroundColor Green
        Wait-Continue
        return
    }
    
    Write-Host "  Zu erstellen: $($itemsToCreate.Count) Issues" -ForegroundColor Cyan
    if ($UseAI) {
        Write-Host "  AI-Optimierung: AKTIV" -ForegroundColor Green
    }
    Write-Host ""
    
    if ($DryRun) {
        Write-Host "  DRY RUN MODE - Keine echten Issues werden erstellt" -ForegroundColor Yellow
        Write-Host ""
    }
    
    $choice = Get-UserChoice -Prompt "Fortfahren?" -Options @("Ja", "Nein") -Default 1
    
    if ($choice -ne 1) {
        Write-Host "  Abgebrochen." -ForegroundColor Yellow
        Wait-Continue
        return
    }
    
    Write-Host ""
    
    $created = 0
    $failed = 0
    $current = 0
    
    foreach ($item in $itemsToCreate) {
        $current++
        
        Write-Host "  [$current/$($itemsToCreate.Count)] $item" -ForegroundColor White
        
        # AI-Optimierung
        if ($UseAI) {
            $optimizedTitle = Optimize-IssueTitle -OriginalTitle $item -ModuleName $ModuleName
        } else {
            $optimizedTitle = $item
        }
        
        if ($DryRun) {
            Write-Host "    [DRY RUN] Wuerde Issue erstellen: [$ModuleName] $optimizedTitle" -ForegroundColor Yellow
            Write-Host ""
            $created++
            Start-Sleep -Milliseconds 100
            continue
        }
        
        # Generiere Body
        $body = @"
**Module:** $ModuleName

**Description:**
$item

**Implementation Tasks:**
- [ ] Research and design approach
- [ ] Implement core functionality
- [ ] Add error handling and validation
- [ ] Write comprehensive unit tests
- [ ] Update module documentation
- [ ] Performance testing and optimization

**Acceptance Criteria:**
- Feature is fully functional and tested
- Tests pass with >80% code coverage
- Documentation is complete and accurate
- No breaking changes to existing APIs
- Performance meets requirements

**Generated by:** AI-powered GitHub Management Script
$(if ($UseAI) { "**AI Model:** $script:OllamaModel" } else { "" })
"@
        
        $suggestedLabels = @($ModuleName, "enhancement", "priority:medium")
        
        $success = New-GitHubIssue `
            -Repo $Repository `
            -Title "[$ModuleName] $optimizedTitle" `
            -Body $body `
            -Labels $suggestedLabels
        
        if ($success) {
            $created++
        } else {
            $failed++
        }
        
        Write-Host ""
        Start-Sleep -Milliseconds 500
    }
    
    Write-Host "  =============================================================" -ForegroundColor Green
    Write-Host "  Erstellt: $created" -ForegroundColor Green
    if ($failed -gt 0) {
        Write-Host "  Fehlgeschlagen: $failed" -ForegroundColor Red
    }
    Write-Host "  =============================================================" -ForegroundColor Green
    
    Wait-Continue
}

# ============================================================================
# ROADMAP STATISTICS
# ============================================================================

function Show-RoadmapStatistics {
    Show-Banner "ROADMAP Statistiken"
    
    $modules = Get-ChildItem $script:SourceDir -Directory | Where-Object {
        Test-Path (Join-Path -Path $_.FullName -ChildPath "ROADMAP.md")
    }
    
    Write-Host "  Analysiere $($modules.Count) Module..." -ForegroundColor Cyan
    Write-Host ""
    
    $totalOpen = 0
    $totalCompleted = 0
    $totalInProgress = 0
    
    foreach ($module in $modules) {
        $roadmapPath = Join-Path -Path $module.FullName -ChildPath "ROADMAP.md"
        $data = Get-RoadmapContent -RoadmapPath $roadmapPath
        
        if ($data) {
            $open = $data.OpenItems.Count
            $completed = $data.CompletedItems.Count
            $inProgress = $data.InProgressItems.Count
            
            $totalOpen += $open
            $totalCompleted += $completed
            $totalInProgress += $inProgress
            
            if ($open -gt 0 -or $inProgress -gt 0) {
                Write-Host "  $($module.Name):" -ForegroundColor White
                Write-Host "    Offen: $open | In Progress: $inProgress | Abgeschlossen: $completed" -ForegroundColor Gray
            }
        }
    }
    
    Write-Host ""
    Write-Host "  =============================================================" -ForegroundColor Cyan
    Write-Host "  GESAMT:" -ForegroundColor Cyan
    Write-Host "    Offen:         $totalOpen" -ForegroundColor Yellow
    Write-Host "    In Progress:   $totalInProgress" -ForegroundColor Cyan
    Write-Host "    Abgeschlossen: $totalCompleted" -ForegroundColor Green
    Write-Host "  =============================================================" -ForegroundColor Cyan
    
    Wait-Continue
}

# ============================================================================
# INITIALISIERUNG
# ============================================================================

function Initialize-System {
    Show-Banner "Initialisierung"
    
    Write-Host "  [1/5] Pruefe Repository..." -NoNewline
    $script:RepoRoot = Find-RepositoryRoot
    Write-Host " OK" -ForegroundColor Green
    Write-Host "        $script:RepoRoot" -ForegroundColor Gray
    
    Write-Host "  [2/5] Pruefe Source-Verzeichnis..." -NoNewline
    if ([string]::IsNullOrWhiteSpace($SourcePath)) {
        $script:SourceDir = Find-SourceDirectory -RepoRoot $script:RepoRoot
    } else {
        $script:SourceDir = $SourcePath
    }
    
    if ($null -eq $script:SourceDir -or -not (Test-Path $script:SourceDir)) {
        Write-Host " FEHLER" -ForegroundColor Red
        Write-Host ""
        Write-Host "  Source-Verzeichnis nicht gefunden!" -ForegroundColor Red
        Write-Host "  Bitte mit -SourcePath Parameter angeben." -ForegroundColor Yellow
        exit 1
    }
    Write-Host " OK" -ForegroundColor Green
    Write-Host "        $script:SourceDir" -ForegroundColor Gray
    
    Write-Host "  [3/5] Pruefe GitHub CLI..." -NoNewline
    if (-not (Test-GitHubCLI)) {
        Write-Host " FEHLER" -ForegroundColor Red
        Write-Host ""
        Write-Host "  GitHub CLI nicht installiert!" -ForegroundColor Red
        Write-Host "  Download: https://cli.github.com/" -ForegroundColor Yellow
        exit 1
    }
    Write-Host " OK" -ForegroundColor Green
    
    Write-Host "  [4/5] Pruefe GitHub Authentifizierung..." -NoNewline
    if (-not (Test-GitHubAuth)) {
        Write-Host " FEHLER" -ForegroundColor Red
        Write-Host ""
        Write-Host "  Nicht authentifiziert!" -ForegroundColor Red
        Write-Host "  Fuehren Sie aus: gh auth login" -ForegroundColor Yellow
        exit 1
    }
    Write-Host " OK" -ForegroundColor Green
    
    Write-Host "  [5/5] Pruefe Ollama..." -NoNewline
    if ($script:UseAI) {
        $ollamaAvailable = Test-OllamaAvailable
        
        if ($ollamaAvailable) {
            Write-Host " OK" -ForegroundColor Green
            Write-Host "        Modell: $script:OllamaModel" -ForegroundColor Gray
        } else {
            Write-Host " NICHT VERFUEGBAR" -ForegroundColor Yellow
            Write-Host "        AI-Features werden deaktiviert" -ForegroundColor Gray
            $script:UseAI = $false
        }
    } else {
        Write-Host " DEAKTIVIERT" -ForegroundColor Gray
    }
    
    Write-Host ""
    Write-Host "  System bereit!" -ForegroundColor Green
    
    Wait-Continue -Message "Druecken Sie Enter um das Hauptmenue zu oeffnen..."
}

# ============================================================================
# HAUPTMENU
# ============================================================================

function Show-MainMenu {
    Show-Banner "GitHub Issue Manager"
    
    Write-Host "  Repository: $Repository" -ForegroundColor Cyan
    Write-Host "  Source: $script:SourceDir" -ForegroundColor Cyan
    Write-Host "  AI Status: " -NoNewline -ForegroundColor Cyan
    
    if ($script:UseAI) {
        $ollamaStatus = if (Test-OllamaAvailable) { "AKTIV ($script:OllamaModel)" } else { "NICHT ERREICHBAR" }
        $color = if (Test-OllamaAvailable) { "Green" } else { "Red" }
        Write-Host $ollamaStatus -ForegroundColor $color
    } else {
        Write-Host "DEAKTIVIERT" -ForegroundColor Gray
    }
    
    if ($DryRun) {
        Write-Host "  Mode: DRY RUN" -ForegroundColor Yellow
    }
    Write-Host ""
    
    Write-Host "  HAUPTMENUE:" -ForegroundColor Green
    Write-Host ""
    Write-Host "    [1] ROADMAP-Statistiken anzeigen" -ForegroundColor Cyan
    Write-Host "    [2] Issues AI-optimiert erstellen" -ForegroundColor Yellow
    Write-Host "    [3] Ollama Einstellungen" -ForegroundColor Cyan
    Write-Host "    [0] Beenden" -ForegroundColor Red
    Write-Host ""
    
    Write-Host "  Ihre Wahl: " -NoNewline -ForegroundColor Yellow
    $input = Read-Host
    
    return $input
}

# ============================================================================
# OLLAMA SETTINGS
# ============================================================================

function Show-OllamaSettings {
    Show-Banner "Ollama Einstellungen"
    
    Write-Host "  Status:" -ForegroundColor Cyan
    Write-Host "    URL: $script:OllamaUrl" -ForegroundColor White
    Write-Host "    Modell: $script:OllamaModel" -ForegroundColor White
    Write-Host "    AI aktiviert: $script:UseAI" -ForegroundColor White
    Write-Host ""
    
    Write-Host "  Pruefe Verbindung..." -NoNewline
    $available = Test-OllamaAvailable
    
    if ($available) {
        Write-Host " OK" -ForegroundColor Green
    } else {
        Write-Host " NICHT ERREICHBAR" -ForegroundColor Red
        Write-Host ""
        Write-Host "  Stellen Sie sicher, dass Ollama laeuft:" -ForegroundColor Yellow
        Write-Host "    ollama serve" -ForegroundColor Gray
    }
    
    Write-Host ""
    Write-Host "  Optionen:" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "    [1] AI aktivieren/deaktivieren" -ForegroundColor White
    Write-Host "    [2] Zurueck" -ForegroundColor Gray
    Write-Host ""
    
    $choice = Get-UserChoice -Prompt "Auswahl:" -Options @("AI toggle", "Zurueck") -Default 2
    
    if ($choice -eq 1) {
        $script:UseAI = -not $script:UseAI
        Write-Host ""
        Write-Host "  AI $( if ($script:UseAI) { 'aktiviert' } else { 'deaktiviert' })" -ForegroundColor Green
        Start-Sleep -Seconds 2
    }
}

# ============================================================================
# HAUPTPROGRAMM
# ============================================================================

function Start-Main {
    Initialize-System
    
    while ($true) {
        $choice = Show-MainMenu
        
        switch ($choice) {
            "1" { Show-RoadmapStatistics }
            "2" { 
                $module = Select-Module -Purpose "AI Issue-Erstellung"
                if ($module) {
                    $roadmapData = Get-RoadmapContent -RoadmapPath $module.RoadmapPath
                    if ($roadmapData) {
                        New-ModuleIssuesAI `
                            -ModuleName $module.Name `
                            -RoadmapData $roadmapData `
                            -UseAI:$script:UseAI `
                            -DryRun:$DryRun
                    }
                }
            }
            "3" { Show-OllamaSettings }
            "0" { 
                Write-Host ""
                Write-Host "  Auf Wiedersehen!" -ForegroundColor Green
                Write-Host ""
                exit 
            }
            default {
                Write-Host "  Ungueltige Eingabe!" -ForegroundColor Red
                Start-Sleep -Seconds 1
            }
        }
    }
}

# ============================================================================
# ENTRY POINT
# ============================================================================

Start-Main