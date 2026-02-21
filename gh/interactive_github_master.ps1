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
    [switch]$UseAI = $true,
    [ValidateSet("interactive", "stats", "create-module-issues", "create-all-issues", "ollama-settings")]
    [string]$Action = "interactive",
    [string]$ModuleName = "",
    [switch]$Yes = $false,
    [switch]$Help = $false,
    [int]$MaxIssues = 0,
    [switch]$EnableMilestones = $true,
    [switch]$EnableRelationships = $true,
    [string]$MilestonePrefix = "",
    [switch]$IncludeUnknownForCreation = $false,
    [string]$OnlyItem = "",
    [string[]]$KeepSubBulletsForStatus = @()
)

# ============================================================================
# GLOBALE VARIABLEN
# ============================================================================

$script:RepoRoot = $null
$script:SourceDir = $null
$script:UseAI = $UseAI
$script:OllamaUrl = $OllamaUrl
$script:OllamaModel = $OllamaModel
$script:IsNonInteractive = ($Action -ne "interactive")
$script:ExitCodes = @{
    Success = 0
    GenericError = 1
    ModuleNotFound = 2
    GitHubAuthFailed = 3
    GitHubCliMissing = 4
    SourceDirMissing = 5
    UnknownAction = 6
    RoadmapReadFailed = 7
}
$script:MilestoneCache = @{}
$script:IssueContextCache = @{}
$script:EnhancementHintCache = @{}
$script:RuleReferenceCache = @{}
$script:KeepSubBulletsForStatus = @($KeepSubBulletsForStatus | ForEach-Object { $_.ToUpperInvariant() })

function Exit-WithCode {
    param(
        [string]$Reason,
        [int]$Code
    )

    if (-not [string]::IsNullOrWhiteSpace($Reason)) {
        Write-Host "  Exit: $Reason (Code: $Code)" -ForegroundColor Red
    }

    exit $Code
}

function Show-Usage {
    Write-Host ""
    Write-Host "  GitHub Issue Manager - Usage" -ForegroundColor Cyan
    Write-Host "  =============================================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  Parameter:" -ForegroundColor Green
    Write-Host "    -Action <interactive|stats|create-module-issues|create-all-issues|ollama-settings>" -ForegroundColor White
    Write-Host "    -ModuleName <name>        (nur fuer create-module-issues)" -ForegroundColor White
    Write-Host "    -SourcePath <path>        (z.B. .\\src)" -ForegroundColor White
    Write-Host "    -Repository <owner/repo>" -ForegroundColor White
    Write-Host "    -DryRun                   (nur simulieren)" -ForegroundColor White
    Write-Host "    -UseAI                    (AI aktivieren)" -ForegroundColor White
    Write-Host "    -Yes                      (auto-bestaetigen)" -ForegroundColor White
    Write-Host "    -EnableMilestones         (Milestones aus Target-Zielen nutzen)" -ForegroundColor White
    Write-Host "    -EnableRelationships      (Related-Work im Body ergaenzen)" -ForegroundColor White
    Write-Host "    -MilestonePrefix <text>   (Fallback-Milestone-Titel ohne Target, versionsbasiert)" -ForegroundColor White
    Write-Host "    -IncludeUnknownForCreation ([!] Eintraege ebenfalls verarbeiten)" -ForegroundColor White
    Write-Host "    -OnlyItem <text>          (nur passendes ROADMAP-Item verarbeiten)" -ForegroundColor White
    Write-Host "    -KeepSubBulletsForStatus  (Statusliste ohne Unterpunkt-Loeschung, z.B. P)" -ForegroundColor White
    Write-Host "    -Help                     (diese Hilfe anzeigen)" -ForegroundColor White
    Write-Host ""
    Write-Host "  Beispiele:" -ForegroundColor Green
    Write-Host "    powershell -NoProfile -ExecutionPolicy Bypass -File .\\gh\\interactive_github_master.ps1 -Action stats -SourcePath .\\src" -ForegroundColor Gray
    Write-Host "    powershell -NoProfile -ExecutionPolicy Bypass -File .\\gh\\interactive_github_master.ps1 -Action create-module-issues -ModuleName acceleration -SourcePath .\\src -DryRun -Yes" -ForegroundColor Gray
    Write-Host "    powershell -NoProfile -ExecutionPolicy Bypass -File .\\gh\\interactive_github_master.ps1 -Action create-module-issues -ModuleName acceleration -SourcePath .\\src -Yes -EnableMilestones -EnableRelationships" -ForegroundColor Gray
    Write-Host "    powershell -NoProfile -ExecutionPolicy Bypass -File .\\gh\\interactive_github_master.ps1 -Action create-module-issues -ModuleName acceleration -SourcePath .\\src -Yes -KeepSubBulletsForStatus P" -ForegroundColor Gray
    Write-Host "    powershell -NoProfile -ExecutionPolicy Bypass -File .\\gh\\interactive_github_master.ps1 -Action create-all-issues -SourcePath .\\src -Yes" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  Exit-Codes:" -ForegroundColor Green
    Write-Host "    0 = Erfolg" -ForegroundColor White
    Write-Host "    1 = Generischer Fehler" -ForegroundColor White
    Write-Host "    2 = Modul nicht gefunden" -ForegroundColor White
    Write-Host "    3 = GitHub Auth fehlgeschlagen" -ForegroundColor White
    Write-Host "    4 = GitHub CLI fehlt" -ForegroundColor White
    Write-Host "    5 = Source-Verzeichnis fehlt" -ForegroundColor White
    Write-Host "    6 = Unbekannte Action" -ForegroundColor White
    Write-Host "    7 = ROADMAP nicht lesbar" -ForegroundColor White
    Write-Host ""
}

function Resolve-MilestoneTitle {
    param(
        [string]$ModuleName,
        [string]$Target,
        [string]$MilestonePrefix = ""
    )

    if (-not [string]::IsNullOrWhiteSpace($Target)) {
        return (($Target.Trim()) -replace "\s+", " ")
    }

    if (-not [string]::IsNullOrWhiteSpace($MilestonePrefix)) {
        return (($MilestonePrefix.Trim()) -replace "\s+", " ")
    }

    return ""
}

function Get-OrCreateMilestoneNumber {
    param(
        [string]$Repo,
        [string]$MilestoneTitle,
        [switch]$DryRun
    )

    if ([string]::IsNullOrWhiteSpace($MilestoneTitle)) {
        return 0
    }

    $cacheKey = "$Repo|$($MilestoneTitle.ToLower())"
    if ($script:MilestoneCache.ContainsKey($cacheKey)) {
        return [int]$script:MilestoneCache[$cacheKey]
    }

    try {
        $existingJson = gh api "repos/$Repo/milestones?state=all&per_page=100" 2>&1
        if ($LASTEXITCODE -eq 0) {
            $milestones = $existingJson | ConvertFrom-Json
            $hit = $milestones | Where-Object { $_.title -ieq $MilestoneTitle } | Select-Object -First 1
            if ($hit) {
                $script:MilestoneCache[$cacheKey] = [int]$hit.number
                return [int]$hit.number
            }
        }

        if ($DryRun) {
            Write-Host "    [DRY RUN] Wuerde Milestone erstellen: $MilestoneTitle" -ForegroundColor Yellow
            return 0
        }

        $createJson = gh api "repos/$Repo/milestones" -X POST -f "title=$MilestoneTitle" -f "state=open" 2>&1
        if ($LASTEXITCODE -eq 0) {
            $created = $createJson | ConvertFrom-Json
            $number = [int]$created.number
            $script:MilestoneCache[$cacheKey] = $number
            Write-Host "    Milestone erstellt: $MilestoneTitle (#$number)" -ForegroundColor Gray
            return $number
        }

        Write-Host "    Milestone konnte nicht erstellt werden: $MilestoneTitle" -ForegroundColor Yellow
        Write-Host "      $createJson" -ForegroundColor DarkYellow
        return 0
    } catch {
        Write-Host "    Milestone-Fehler: $_" -ForegroundColor Yellow
        return 0
    }
}

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

    if ($script:IsNonInteractive) {
        return
    }
    
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

function Get-ExistingLabels {
    param([string]$Repo)

    try {
        $labelsJson = gh label list --repo $Repo --limit 1000 --json name 2>&1
        if ($LASTEXITCODE -ne 0) {
            return @()
        }

        $labels = $labelsJson | ConvertFrom-Json
        return @($labels | ForEach-Object { $_.name })
    } catch {
        return @()
    }
}

function Get-LabelColor {
    param([string]$LabelName)

    if ($LabelName -eq "enhancement") { return "a2eeef" }
    if ($LabelName -like "priority:*") { return "d93f0b" }

    $hash = [Math]::Abs($LabelName.GetHashCode())
    $palette = @("1d76db", "0052cc", "5319e7", "0e8a16", "006b75", "fbca04")
    return $palette[$hash % $palette.Count]
}

function Ensure-LabelsExist {
    param(
        [string]$Repo,
        [array]$Labels,
        [switch]$DryRun
    )

    $cleanLabels = @($Labels | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
    if ($cleanLabels.Count -eq 0) {
        return $true
    }

    Write-Host "  Pruefe Labels..." -NoNewline -ForegroundColor Cyan
    $existing = Get-ExistingLabels -Repo $Repo
    $existingSet = @{}

    foreach ($name in $existing) {
        $existingSet[$name.ToLower()] = $true
    }

    $missing = @($cleanLabels | Where-Object { -not $existingSet.ContainsKey($_.ToLower()) })

    if ($missing.Count -eq 0) {
        Write-Host " OK" -ForegroundColor Green
        return $true
    }

    Write-Host " $($missing.Count) fehlen" -ForegroundColor Yellow

    foreach ($label in $missing) {
        if ($DryRun) {
            Write-Host "    [DRY RUN] Wuerde Label erstellen: $label" -ForegroundColor Yellow
            continue
        }

        $color = Get-LabelColor -LabelName $label
        $description = "Auto-created by issue manager"

        $result = gh label create "$label" --repo $Repo --color $color --description "$description" 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "    Label erstellt: $label" -ForegroundColor Green
        } else {
            if ("$result" -match "already exists") {
                Write-Host "    Label existiert bereits: $label" -ForegroundColor Gray
            } else {
                Write-Host "    Label konnte nicht erstellt werden: $label" -ForegroundColor Red
                Write-Host "      $result" -ForegroundColor DarkRed
                return $false
            }
        }
    }

    return $true
}

function Get-ExistingIssuesForModule {
    param([string]$Repo, [string]$ModuleName)
    
    Write-Host "  Pruefe vorhandene Issues/PRs fuer Modul '$ModuleName'..." -NoNewline
    
    try {
        $issuesJson = gh issue list `
            --repo $Repo `
            --state all `
            --search "[$ModuleName] in:title" `
            --limit 200 `
            --json number,title,state,labels,url 2>&1
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host " FEHLER" -ForegroundColor Red
            Write-Host "    $issuesJson" -ForegroundColor Red
            return @()
        }

        $prsJson = gh pr list `
            --repo $Repo `
            --state all `
            --search "[$ModuleName] in:title" `
            --limit 200 `
            --json number,title,state,url 2>&1

        if ($LASTEXITCODE -ne 0) {
            Write-Host " FEHLER" -ForegroundColor Red
            Write-Host "    $prsJson" -ForegroundColor Red
            return @()
        }
        
        $issues = @($issuesJson | ConvertFrom-Json)
        $prs = @($prsJson | ConvertFrom-Json)

        foreach ($issue in $issues) {
            $issue | Add-Member -NotePropertyName WorkType -NotePropertyValue "issue" -Force
        }

        foreach ($pr in $prs) {
            $pr | Add-Member -NotePropertyName WorkType -NotePropertyValue "pr" -Force
        }

        $existingWork = @($issues) + @($prs)
        
        Write-Host " $($existingWork.Count) gefunden (Issues: $($issues.Count), PRs: $($prs.Count))" -ForegroundColor Green
        
        return $existingWork
        
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

    $Normalize = {
        param([string]$Text)

        if ([string]::IsNullOrWhiteSpace($Text)) {
            return ""
        }

        return (($Text.ToLower() -replace '[^a-z0-9\s]', ' ') -replace '\s+', ' ').Trim()
    }

    $GetTokens = {
        param([string]$Text)

        $stopWords = @(
            'the', 'and', 'for', 'with', 'from', 'into', 'onto', 'this', 'that',
            'module', 'feature', 'task', 'tests', 'test', 'api', 'backend'
        )

        $tokenSet = @{}
        $words = (& $Normalize $Text) -split ' '

        foreach ($word in $words) {
            if ($word.Length -lt 3) { continue }
            if ($stopWords -contains $word) { continue }
            $tokenSet[$word] = $true
        }

        return $tokenSet
    }

    $GetJaccard = {
        param([hashtable]$A, [hashtable]$B)

        $union = @{}
        foreach ($k in $A.Keys) { $union[$k] = $true }
        foreach ($k in $B.Keys) { $union[$k] = $true }

        if ($union.Count -eq 0) { return 0.0 }

        $intersectionCount = 0
        foreach ($k in $A.Keys) {
            if ($B.ContainsKey($k)) {
                $intersectionCount++
            }
        }

        return [double]$intersectionCount / [double]$union.Count
    }

    $normalizedSearch = & $Normalize $ItemTitle
    $searchTokens = & $GetTokens $ItemTitle
    $bestCandidate = $null
    $bestSimilarity = 0.0
    
    foreach ($issue in $ExistingIssues) {
        $issueTitle = $issue.title -replace '^\[.+?\]\s*', ''

        $normalizedIssue = & $Normalize $issueTitle

        if ([string]::IsNullOrWhiteSpace($normalizedIssue)) {
            continue
        }

        if ($normalizedSearch -eq $normalizedIssue) {
            return @{
                Exists = $true
                Uncertain = $false
                Issue = $issue
                Similarity = 1.0
            }
        }

        if ($normalizedIssue.Contains($normalizedSearch) -or $normalizedSearch.Contains($normalizedIssue)) {
            $shortLen = [Math]::Min($normalizedSearch.Length, $normalizedIssue.Length)
            $longLen = [Math]::Max($normalizedSearch.Length, $normalizedIssue.Length)
            $ratio = if ($longLen -gt 0) { [double]$shortLen / [double]$longLen } else { 0.0 }

            if ($shortLen -ge 20 -and $ratio -ge 0.90) {
                return @{
                    Exists = $true
                    Uncertain = $false
                    Issue = $issue
                    Similarity = $ratio
                }
            }

            if ($ratio -gt $bestSimilarity) {
                $bestSimilarity = $ratio
                $bestCandidate = $issue
            }
        }

        $issueTokens = & $GetTokens $issueTitle
        $jaccard = & $GetJaccard $searchTokens $issueTokens

        if ($jaccard -ge 0.78) {
            return @{
                Exists = $true
                Uncertain = $false
                Issue = $issue
                Similarity = $jaccard
            }
        }

        if ($jaccard -gt $bestSimilarity) {
            $bestSimilarity = $jaccard
            $bestCandidate = $issue
        }
    }

    if ($bestSimilarity -ge 0.55 -and $bestSimilarity -lt 0.78) {
        return @{
            Exists = $false
            Uncertain = $true
            Issue = $bestCandidate
            Similarity = $bestSimilarity
        }
    }
    
    return @{
        Exists = $false
        Uncertain = $false
        Issue = $null
        Similarity = 0
    }
}

function Get-NormalizedRoadmapCompareText {
    param([string]$Text)

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return ""
    }

    $normalized = $Text
    $normalized = $normalized -replace '\(Target:.*?\)', ''
    $normalized = $normalized -replace '\((Issue|PR):\s*#\d+\)', ''
    $normalized = $normalized -replace '\((Issue|PR)\s*#\d+\)', ''
    $normalized = $normalized -replace '[^a-zA-Z0-9\s\-/]', ' '
    $normalized = ($normalized.ToLower() -replace '\s+', ' ').Trim()

    return $normalized
}

function Set-RoadmapItemStatus {
    param(
        [string]$RoadmapPath,
        [string]$ItemTitle,
        [string]$NewStatus,
        [string]$ReferenceType = "",
        [int]$ReferenceNumber = 0,
        [switch]$DryRun
    )

    if ([string]::IsNullOrWhiteSpace($RoadmapPath) -or -not (Test-Path $RoadmapPath)) {
        return $false
    }

    $rawContent = Get-Content $RoadmapPath -Raw -Encoding UTF8
    if ($null -eq $rawContent) {
        return $false
    }

    $lineEnding = if ($rawContent -match "`r`n") { "`r`n" } else { "`n" }
    $hasTrailingNewline = ($rawContent.EndsWith("`r`n") -or $rawContent.EndsWith("`n"))
    $lines = @($rawContent -split "`r?`n")
    if ($hasTrailingNewline -and $lines.Count -gt 0 -and $lines[$lines.Count - 1] -eq "") {
        $lines = $lines[0..($lines.Count - 2)]
    }

    if ($lines.Count -eq 0) {
        return $false
    }

    $target = Get-NormalizedRoadmapCompareText -Text $ItemTitle
    if ([string]::IsNullOrWhiteSpace($target)) {
        return $false
    }

    $matchedIndex = -1
    $matchedText = ""

    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        $match = [regex]::Match($line, '^\s*-\s+\[[^\]]\]\s+(.+?)\s*$')
        if (-not $match.Success) {
            continue
        }

        $lineText = $match.Groups[1].Value
        $lineNormalized = Get-NormalizedRoadmapCompareText -Text $lineText

        if ($lineNormalized -eq $target) {
            $matchedIndex = $i
            $matchedText = $lineText
            break
        }
    }

    if ($matchedIndex -lt 0) {
        return $false
    }

    $baseText = $matchedText -replace '\s*\((Issue|PR):\s*#\d+\)\s*$', ''
    $baseText = $baseText -replace '\s*\((Issue|PR)\s*#\d+\)\s*$', ''
    $baseText = $baseText.TrimEnd()

    $referenceSuffix = ""
    if (-not [string]::IsNullOrWhiteSpace($ReferenceType) -and $ReferenceNumber -gt 0) {
        $referenceSuffix = " ($($ReferenceType): #$ReferenceNumber)"
    }

    $updatedLine = "- [$NewStatus] $baseText$referenceSuffix"

    $detailLinesRemoved = 0
    $statusUpper = $NewStatus.ToUpperInvariant()
    $keepDetailsForStatus = $script:KeepSubBulletsForStatus -contains $statusUpper

    if (($NewStatus -eq "I" -or $NewStatus -eq "P") -and -not $keepDetailsForStatus) {
        $lineMatch = [regex]::Match($lines[$matchedIndex], '^(\s*)-\s+\[[^\]]\]\s+')
        $parentIndent = if ($lineMatch.Success) { $lineMatch.Groups[1].Value.Length } else { 0 }

        $removeStart = $matchedIndex + 1
        $removeEnd = $matchedIndex
        $removalActive = $false

        for ($j = $removeStart; $j -lt $lines.Count; $j++) {
            $nextLine = $lines[$j]

            if ($nextLine -match '^\s*#{1,6}\s+') {
                break
            }

            $nextChecklist = [regex]::Match($nextLine, '^(\s*)-\s+\[[^\]]\]\s+')
            if ($nextChecklist.Success -and $nextChecklist.Groups[1].Value.Length -le $parentIndent) {
                break
            }

            $indentMatch = [regex]::Match($nextLine, '^(\s*)')
            $nextIndent = $indentMatch.Groups[1].Value.Length

            $isNestedBullet = ($nextIndent -gt $parentIndent) -and ($nextLine -match '^\s*[-*]\s+' -or $nextLine -match '^\s*\d+\.\s+')

            if ($isNestedBullet) {
                $removalActive = $true
                $removeEnd = $j
                continue
            }

            if ($removalActive) {
                if ([string]::IsNullOrWhiteSpace($nextLine) -or $nextIndent -gt $parentIndent) {
                    $removeEnd = $j
                    continue
                }
                break
            }

            if ([string]::IsNullOrWhiteSpace($nextLine)) {
                continue
            }

            break
        }

        if ($removeEnd -ge $removeStart) {
            $detailLinesRemoved = $removeEnd - $removeStart + 1
            $head = @($lines[0..$matchedIndex])
            $tail = @()
            if (($removeEnd + 1) -le ($lines.Count - 1)) {
                $tail = @($lines[($removeEnd + 1)..($lines.Count - 1)])
            }
            $lines = @($head + $tail)
        }
    }

    if (($NewStatus -eq "I" -or $NewStatus -eq "P") -and $keepDetailsForStatus) {
        Write-Host "    Unterpunkte bleiben erhalten fuer Status [$statusUpper]" -ForegroundColor DarkGray
    }

    if ($DryRun) {
        Write-Host "    [DRY RUN] Wuerde ROADMAP aktualisieren: $updatedLine" -ForegroundColor Yellow
        if ($detailLinesRemoved -gt 0) {
            Write-Host "    [DRY RUN] Wuerde Unterpunkte entfernen: $detailLinesRemoved" -ForegroundColor Yellow
        }
        return $true
    }

    $lines[$matchedIndex] = $updatedLine

    $newContent = ($lines -join $lineEnding)
    if ($hasTrailingNewline) {
        $newContent += $lineEnding
    }

    $hasBom = $false
    try {
        $fs = [System.IO.File]::OpenRead($RoadmapPath)
        try {
            $b0 = $fs.ReadByte()
            $b1 = $fs.ReadByte()
            $b2 = $fs.ReadByte()
            $hasBom = ($b0 -eq 239 -and $b1 -eq 187 -and $b2 -eq 191)
        } finally {
            $fs.Dispose()
        }
    } catch {
    }

    $utf8 = New-Object System.Text.UTF8Encoding($hasBom)
    [System.IO.File]::WriteAllText($RoadmapPath, $newContent, $utf8)

    Write-Host "    ROADMAP aktualisiert: $updatedLine" -ForegroundColor Gray
    if ($detailLinesRemoved -gt 0) {
        Write-Host "    Unterpunkte entfernt: $detailLinesRemoved" -ForegroundColor Gray
    }
    return $true
}

function New-GitHubIssue {
    param(
        [string]$Repo,
        [string]$Title,
        [string]$Body,
        [array]$Labels = @(),
        [int]$MilestoneNumber = 0
    )
    
    try {
        $cleanLabels = @($Labels | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)

        $payload = @{
            title = $Title
            body = $Body
        }

        if ($cleanLabels.Count -gt 0) {
            $payload.labels = $cleanLabels
        }

        if ($MilestoneNumber -gt 0) {
            $payload.milestone = $MilestoneNumber
        }

        $json = $payload | ConvertTo-Json -Depth 5

        Write-Host "    Erstelle Issue..." -NoNewline -ForegroundColor Cyan

        $bodyPreviewLines = @(($Body -split "`r?`n") | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -First 3)
        if ($bodyPreviewLines.Count -gt 0) {
            Write-Host "" 
            Write-Host "    Body-Vorschau:" -ForegroundColor DarkGray
            foreach ($line in $bodyPreviewLines) {
                Write-Host "      $line" -ForegroundColor DarkGray
            }
        }

        $result = $json | gh api "repos/$Repo/issues" -X POST --input - 2>&1

        if ($LASTEXITCODE -eq 0) {
            Write-Host " ERFOLG!" -ForegroundColor Green
            
            try {
                $issueData = $result | ConvertFrom-Json
                Write-Host "    Issue #$($issueData.number) erstellt: $($issueData.html_url)" -ForegroundColor Gray
            } catch {
                Write-Host "    Issue erstellt" -ForegroundColor Gray
            }
            
            $issueNumber = 0
            $issueUrl = ""

            try {
                $issueData = $result | ConvertFrom-Json
                $issueNumber = [int]$issueData.number
                $issueUrl = "$($issueData.html_url)"
            } catch {
            }

            return @{
                Success = $true
                Number = $issueNumber
                Url = $issueUrl
                Error = ""
            }
        } else {
            $errorText = "$result"

            if ($cleanLabels.Count -gt 0 -and ($errorText -match 'Validation Failed' -or $errorText -match 'label' -or $errorText -match 'unprocessable')) {
                Write-Host " FEHLER (Label-Problem, Retry ohne Labels)..." -ForegroundColor Yellow

                $fallbackPayload = @{
                    title = $Title
                    body = $Body
                } | ConvertTo-Json -Depth 5

                if ($MilestoneNumber -gt 0) {
                    $fallbackPayloadObj = @{
                        title = $Title
                        body = $Body
                        milestone = $MilestoneNumber
                    }
                    $fallbackPayload = $fallbackPayloadObj | ConvertTo-Json -Depth 5
                }

                $fallbackResult = $fallbackPayload | gh api "repos/$Repo/issues" -X POST --input - 2>&1

                if ($LASTEXITCODE -eq 0) {
                    Write-Host " ERFOLG (ohne Labels)!" -ForegroundColor Green
                    try {
                        $issueData = $fallbackResult | ConvertFrom-Json
                        Write-Host "    Issue #$($issueData.number) erstellt: $($issueData.html_url)" -ForegroundColor Gray
                    } catch {
                        Write-Host "    Issue erstellt (ohne Labels)" -ForegroundColor Gray
                    }
                    $issueNumber = 0
                    $issueUrl = ""
                    try {
                        $issueData = $fallbackResult | ConvertFrom-Json
                        $issueNumber = [int]$issueData.number
                        $issueUrl = "$($issueData.html_url)"
                    } catch {
                    }

                    return @{
                        Success = $true
                        Number = $issueNumber
                        Url = $issueUrl
                        Error = ""
                    }
                }

                Write-Host " FEHLER!" -ForegroundColor Red
                Write-Host "    Erstversuch: $errorText" -ForegroundColor DarkRed
                Write-Host "    Fallback ohne Labels: $fallbackResult" -ForegroundColor Red
                return @{
                    Success = $false
                    Number = 0
                    Url = ""
                    Error = "Fallback without labels failed"
                }
            }

            Write-Host " FEHLER!" -ForegroundColor Red
            Write-Host "    GitHub API Error: $errorText" -ForegroundColor Red
            return @{
                Success = $false
                Number = 0
                Url = ""
                Error = "$errorText"
            }
        }
        
    } catch {
        Write-Host " EXCEPTION!" -ForegroundColor Red
        Write-Host "    Error: $_" -ForegroundColor Red
        return @{
            Success = $false
            Number = 0
            Url = ""
            Error = "$_"
        }
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
    $issueItems = @()
    $pullRequestItems = @()
    $questionItems = @()
    $unknownItems = @()
    $targetByItem = @{}
    
    $checkboxPattern = '-\s+\[([^\]])\]\s+(.+?)(?:\s*\(Target:\s*(.*?)\))?\s*$'
    $matches = [regex]::Matches($content, $checkboxPattern, 'Multiline')
    
    foreach ($match in $matches) {
        $status = $match.Groups[1].Value
        $itemText = $match.Groups[2].Value.Trim()
        $targetText = $match.Groups[3].Value.Trim()

        if (-not [string]::IsNullOrWhiteSpace($targetText) -and -not $targetByItem.ContainsKey($itemText)) {
            $targetByItem[$itemText] = $targetText
        }
        
        switch ($status) {
            ' ' { $openItems += $itemText }
            'x' { $completedItems += $itemText }
            'X' { $completedItems += $itemText }
            '~' { $inProgressItems += $itemText }
            'I' { $issueItems += $itemText }
            'i' { $issueItems += $itemText }
            'P' { $pullRequestItems += $itemText }
            'p' { $pullRequestItems += $itemText }
            '?' { $questionItems += $itemText }
            '!' { $unknownItems += $itemText }
            default { $unknownItems += $itemText }
        }
    }
    
    return @{
        OpenItems = $openItems
        InProgressItems = $inProgressItems
        CompletedItems = $completedItems
        IssueItems = $issueItems
        PullRequestItems = $pullRequestItems
        QuestionItems = $questionItems
        UnknownItems = $unknownItems
        TargetByItem = $targetByItem
        AllItems = $openItems + $inProgressItems
    }
}

function Get-SectionLines {
    param(
        [array]$Lines,
        [string]$HeadingRegex
    )

    $start = -1
    for ($i = 0; $i -lt $Lines.Count; $i++) {
        if ($Lines[$i] -match $HeadingRegex) {
            $start = $i
            break
        }
    }

    if ($start -lt 0) {
        return @()
    }

    $result = @()
    for ($j = $start + 1; $j -lt $Lines.Count; $j++) {
        if ($Lines[$j] -match '^\s*#{2,6}\s+') {
            break
        }
        $result += $Lines[$j]
    }

    return $result
}

function Get-ModuleEnhancementHints {
    param(
        [string]$ModuleName,
        [string]$RoadmapPath
    )

    $cacheKey = "$($ModuleName.ToLower())|$RoadmapPath"
    if ($script:EnhancementHintCache.ContainsKey($cacheKey)) {
        return $script:EnhancementHintCache[$cacheKey]
    }

    $hints = @()
    $moduleDir = Split-Path -Path $RoadmapPath -Parent
    $candidates = @(
        (Join-Path $moduleDir "future_enhancement.md"),
        (Join-Path $moduleDir "future_enhancements.md"),
        (Join-Path $moduleDir "feature_enhancement.md"),
        (Join-Path $script:RepoRoot "future_enhancement.md"),
        (Join-Path $script:RepoRoot "future_enhancements.md"),
        (Join-Path $script:RepoRoot "feature_enhancement.md")
    ) | Select-Object -Unique

    foreach ($candidate in $candidates) {
        if (-not (Test-Path $candidate)) { continue }

        try {
            $lines = @(Get-Content $candidate -Encoding UTF8)
            if ($lines.Count -eq 0) { continue }

            $moduleHeadingRegex = "^\s*#{1,6}\s+.*$([regex]::Escape($ModuleName)).*$"
            $section = Get-SectionLines -Lines $lines -HeadingRegex $moduleHeadingRegex

            $picked = @()
            if ($section.Count -gt 0) {
                $picked = @($section | Where-Object {
                    $_ -match '^\s*[-*]\s+' -or $_ -match '^\s*\d+\.\s+'
                } | ForEach-Object {
                    ($_ -replace '^\s*[-*]\s+', '' -replace '^\s*\d+\.\s+', '').Trim()
                } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -First 8)
            }

            if ($picked.Count -eq 0) {
                $moduleNeedle = $ModuleName.ToLower()
                $picked = @($lines | Where-Object {
                    $_.ToLower().Contains($moduleNeedle) -and (
                        $_ -match 'TODO|FIXME|stub|implement|phase|acceptance|criterion|milestone'
                    )
                } | ForEach-Object { $_.Trim() } | Select-Object -First 5)
            }

            $hints += $picked
        } catch {
        }
    }

    $hints = @($hints | Select-Object -Unique | Select-Object -First 10)
    $script:EnhancementHintCache[$cacheKey] = $hints
    return $hints
}

function Get-RoadmapIssueContext {
    param(
        [string]$RoadmapPath,
        [string]$ModuleName,
        [string]$ItemTitle,
        [string]$Target
    )

    $cacheKey = "$RoadmapPath|$ItemTitle"
    if ($script:IssueContextCache.ContainsKey($cacheKey)) {
        return $script:IssueContextCache[$cacheKey]
    }

    $empty = @{
        SectionTitle = ""
        PhaseSteps = @()
        AcceptanceCriteria = @()
        KnownLimitations = @()
        BreakingChanges = @()
        EnhancementHints = @()
        Target = $Target
    }

    if (-not (Test-Path $RoadmapPath)) {
        return $empty
    }

    try {
        $lines = @(Get-Content $RoadmapPath -Encoding UTF8)
        if ($lines.Count -eq 0) {
            return $empty
        }

        $itemIndex = -1
        $targetNorm = Get-NormalizedRoadmapCompareText -Text $ItemTitle

        for ($i = 0; $i -lt $lines.Count; $i++) {
            $m = [regex]::Match($lines[$i], '^\s*-\s+\[[^\]]\]\s+(.+?)\s*$')
            if (-not $m.Success) { continue }

            $lineText = $m.Groups[1].Value
            $norm = Get-NormalizedRoadmapCompareText -Text $lineText
            if ($norm -eq $targetNorm) {
                $itemIndex = $i
                break
            }
        }

        $sectionTitle = ""
        if ($itemIndex -ge 0) {
            for ($j = $itemIndex; $j -ge 0; $j--) {
                if ($lines[$j] -match '^\s*#{2,6}\s+(.+?)\s*$') {
                    $sectionTitle = $matches[1].Trim()
                    break
                }
            }
        }

        $phaseSteps = @()
        for ($p = 0; $p -lt $lines.Count; $p++) {
            if ($lines[$p] -match '^\s*#{2,6}\s*(Phase\s+\d+.*)\s*$') {
                $phaseTitle = $matches[1].Trim()
                $phaseItems = @()

                for ($k = $p + 1; $k -lt $lines.Count; $k++) {
                    if ($lines[$k] -match '^\s*#{2,6}\s+') {
                        break
                    }

                    if ($lines[$k] -match '^\s*[-*]\s+' -or $lines[$k] -match '^\s*\d+\.\s+') {
                        $clean = ($lines[$k] -replace '^\s*[-*]\s+', '' -replace '^\s*\d+\.\s+', '').Trim()
                        if (-not [string]::IsNullOrWhiteSpace($clean)) {
                            $phaseItems += $clean
                        }
                    }
                }

                if ($phaseItems.Count -gt 0) {
                    $phaseSteps += "$($phaseTitle): $($phaseItems -join '; ')"
                } else {
                    $phaseSteps += $phaseTitle
                }
            }
        }

        $acceptanceLines = Get-SectionLines -Lines $lines -HeadingRegex '^\s*#{2,6}\s*Production Readiness Checklist\s*$'
        $acceptance = @($acceptanceLines | Where-Object { $_ -match '^\s*-\s+\[[^\]]\]\s+' -or $_ -match '^\s*[-*]\s+' } | ForEach-Object {
            ($_ -replace '^\s*-\s+\[[^\]]\]\s+', '' -replace '^\s*[-*]\s+', '').Trim()
        } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -First 8)

        $knownLines = Get-SectionLines -Lines $lines -HeadingRegex '^\s*#{2,6}\s*Known Issues\s*&\s*Limitations\s*$'
        $known = @($knownLines | Where-Object { $_ -match '^\s*[-*]\s+' } | ForEach-Object {
            ($_ -replace '^\s*[-*]\s+', '').Trim()
        } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -First 6)

        $breakingLines = Get-SectionLines -Lines $lines -HeadingRegex '^\s*#{2,6}\s*Breaking Changes\s*$'
        $breaking = @($breakingLines | Where-Object { $_ -match '^\s*[-*]\s+' } | ForEach-Object {
            ($_ -replace '^\s*[-*]\s+', '').Trim()
        } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -First 6)

        $hints = Get-ModuleEnhancementHints -ModuleName $ModuleName -RoadmapPath $RoadmapPath

        $ctx = @{
            SectionTitle = $sectionTitle
            PhaseSteps = $phaseSteps
            AcceptanceCriteria = $acceptance
            KnownLimitations = $known
            BreakingChanges = $breaking
            EnhancementHints = $hints
            Target = $Target
        }

        $script:IssueContextCache[$cacheKey] = $ctx
        return $ctx
    } catch {
        return $empty
    }
}

function Get-DesignRuleReferences {
    param([string]$ModuleName)

    $moduleKey = $ModuleName.ToLower()
    if ($script:RuleReferenceCache.ContainsKey($moduleKey)) {
        return $script:RuleReferenceCache[$moduleKey]
    }

    $refs = @()
    $priority = @(
        "docs/analysis/IMPLEMENTATION_GUIDE.md",
        "docs/analysis/$($ModuleName.ToUpper())_ACCELERATION_ADDENDUM.md",
        "docs/analysis/GPU_ACCELERATION_ADDENDUM.md",
        "docs/architecture/MODULAR_ARCHITECTURE_ROADMAP.md",
        "docs/architecture/THEMIS_CORE_GUIDE.md"
    )

    foreach ($rel in $priority) {
        $abs = Join-Path $script:RepoRoot $rel
        if (Test-Path $abs) {
            $refs += $rel
        }
    }

    try {
        $docFiles = Get-ChildItem (Join-Path $script:RepoRoot "docs") -Recurse -File -Filter "*.md" -ErrorAction SilentlyContinue
        $moduleDocs = @($docFiles | Where-Object {
            $p = $_.FullName.ToLower()
            ($p -like "*$moduleKey*") -and (
                $p -match "implementation|design|guide|rules|addendum|architecture"
            )
        } | Select-Object -First 6)

        foreach ($f in $moduleDocs) {
            $relPath = $f.FullName.Substring($script:RepoRoot.Length + 1).Replace('\\','/')
            $refs += $relPath
        }
    } catch {
    }

    $refs = @($refs | Select-Object -Unique | Select-Object -First 8)
    $script:RuleReferenceCache[$moduleKey] = $refs
    return $refs
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

function Get-ModuleByName {
    param([string]$Name)

    if ([string]::IsNullOrWhiteSpace($Name)) {
        return $null
    }

    $modules = Get-ChildItem $script:SourceDir -Directory | Where-Object {
        Test-Path (Join-Path -Path $_.FullName -ChildPath "ROADMAP.md")
    }

    $module = $modules | Where-Object { $_.Name -ieq $Name } | Select-Object -First 1
    if (-not $module) {
        $module = $modules | Where-Object { $_.Name -like "*$Name*" } | Select-Object -First 1
    }

    if (-not $module) {
        return $null
    }

    return @{
        Name = $module.Name
        Path = $module.FullName
        RoadmapPath = Join-Path -Path $module.FullName -ChildPath "ROADMAP.md"
    }
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
        [string]$Model = $script:OllamaModel,
        [double]$Temperature = 0.7,
        [int]$NumPredict = 500
    )
    
    try {
        $body = @{
            model = $Model
            messages = @(
                @{ role = "system"; content = $SystemPrompt },
                @{ role = "user"; content = $UserPrompt }
            )
            stream = $false
            options = @{ temperature = $Temperature; num_predict = $NumPredict }
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
        [string]$ModuleName,
        [string]$RoadmapMarker = "[ ]",
        [switch]$IsUncertain
    )

    $statusLegend = @"
[x] = erledigt
[I] = issue vorhanden
[P] = pull request
[?] = human question
[!] = unbekannter status
[ ] = offen
[~] = in bearbeitung
"@

    $systemPrompt = @"
You are a GitHub issue title optimizer for roadmap-driven automation.

Roadmap status legend:
$statusLegend

Rules:
1) Return EXACTLY ONE LINE containing ONLY the title text.
2) Never return labels like "Title:", "Optimized title:", "Issue:", "Output:".
3) Never return quotes, markdown, bullets, numbering, backticks, or explanations.
4) Imperative mood, specific, actionable, <= 80 characters when possible.
5) Preserve technical meaning; do not invent scope.
6) Do NOT include module prefix like [module] in your output.
7) If uncertainty is true, do NOT add [!]; caller handles marker prefixing.

Invalid output examples:
- Optimized title: Improve CUDA kernels
- "Implement Vulkan fallback"
- 1) Implement Vulkan fallback

Valid output example:
Implement Vulkan fallback for non-NVIDIA hardware
"@

    $uncertaintyText = if ($IsUncertain) { "true" } else { "false" }

    $userPrompt = @"
Module: $ModuleName
Roadmap marker: $RoadmapMarker
Uncertainty flag: $uncertaintyText
Original title: $OriginalTitle

Optimize this GitHub issue title according to the rules.
"@
    
    Write-Host "    AI optimiert Titel..." -NoNewline -ForegroundColor Cyan
    
    $optimized = Invoke-OllamaChat `
        -SystemPrompt $systemPrompt `
        -UserPrompt $userPrompt `
        -Temperature 0.2 `
        -NumPredict 120
    
    if ($optimized) {
        $optimized = $optimized.Trim()
        $optimized = $optimized -replace '^[`"'']?(optimized\s+title|title)\s*:\s*', ''
        $optimized = $optimized.Trim().Trim('"').Trim("'")
        $optimized = ($optimized -replace '\s+', ' ').Trim()
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
        [string]$RoadmapPath,
        [switch]$UseAI,
        [switch]$DryRun,
        [switch]$AutoConfirm,
        [int]$MaxIssues = 0,
        [switch]$EnableMilestones,
        [switch]$EnableRelationships,
        [string]$MilestonePrefix = "",
        [switch]$IncludeUnknownForCreation,
        [string]$OnlyItem = ""
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
    $targetByItem = @{}
    if ($RoadmapData.ContainsKey("TargetByItem") -and $RoadmapData.TargetByItem) {
        $targetByItem = $RoadmapData.TargetByItem
    }

    if ($RoadmapData.IssueItems.Count -gt 0 -or $RoadmapData.PullRequestItems.Count -gt 0 -or $RoadmapData.QuestionItems.Count -gt 0 -or $RoadmapData.UnknownItems.Count -gt 0) {
        Write-Host "  ROADMAP Marker:" -ForegroundColor Cyan
        if ($RoadmapData.IssueItems.Count -gt 0) {
            Write-Host "    [I] Issue vorhanden: $($RoadmapData.IssueItems.Count)" -ForegroundColor Gray
        }
        if ($RoadmapData.PullRequestItems.Count -gt 0) {
            Write-Host "    [P] Pull Request: $($RoadmapData.PullRequestItems.Count)" -ForegroundColor Gray
        }
        if ($RoadmapData.QuestionItems.Count -gt 0) {
            Write-Host "    [?] Human Question: $($RoadmapData.QuestionItems.Count)" -ForegroundColor Yellow
        }
        if ($RoadmapData.UnknownItems.Count -gt 0) {
            Write-Host "    [!] Unbekannt: $($RoadmapData.UnknownItems.Count)" -ForegroundColor Yellow
        }
        Write-Host ""
    }
    $uncertainCount = 0
    $preExistingUpdated = 0
    
    $candidateItems = @($RoadmapData.AllItems)
    if ($IncludeUnknownForCreation -and $RoadmapData.UnknownItems.Count -gt 0) {
        Write-Host "  [!] IncludeUnknownForCreation aktiv: $($RoadmapData.UnknownItems.Count) Eintraege werden geprueft" -ForegroundColor Yellow
        $candidateItems += $RoadmapData.UnknownItems
    }

    if (-not [string]::IsNullOrWhiteSpace($OnlyItem)) {
        $needle = $OnlyItem.ToLower()
        $candidateItems = @($candidateItems | Where-Object { $_.ToLower().Contains($needle) })
        Write-Host "  OnlyItem Filter aktiv: '$OnlyItem' -> $($candidateItems.Count) Treffer" -ForegroundColor Yellow
    }

    foreach ($item in $candidateItems) {
        $check = Test-IssueExists -ModuleName $ModuleName -ItemTitle $item -ExistingIssues $existingIssues

        if ($check.Exists -and $check.Issue) {
            $isPr = ($check.Issue.WorkType -eq "pr")
            $status = if ($isPr) { "P" } else { "I" }
            $refType = if ($isPr) { "PR" } else { "Issue" }
            $refNumber = if ($check.Issue.number) { [int]$check.Issue.number } else { 0 }

            $updated = Set-RoadmapItemStatus `
                -RoadmapPath $RoadmapPath `
                -ItemTitle $item `
                -NewStatus $status `
                -ReferenceType $refType `
                -ReferenceNumber $refNumber `
                -DryRun:$DryRun

            if ($updated) {
                $preExistingUpdated++
            }
            continue
        }
        
        $isUncertain = $false
        if ($check.ContainsKey("Uncertain") -and $check.Uncertain) {
            $isUncertain = $true
            $uncertainCount++
            $candidateType = if ($check.Issue -and $check.Issue.WorkType) { $check.Issue.WorkType.ToUpper() } else { "WORK" }
            $candidateNumber = if ($check.Issue -and $check.Issue.number) { "#$($check.Issue.number)" } else { "" }
            Write-Host "  [!] Unklarer Treffer fuer '$item' -> $candidateType $candidateNumber (Score: $([Math]::Round($check.Similarity, 2)))" -ForegroundColor Yellow
        }

        $itemsToCreate += @{
            Text = $item
            Uncertain = $isUncertain
            Similarity = $check.Similarity
            Candidate = $check.Issue
            Target = $(if ($targetByItem.ContainsKey($item)) { $targetByItem[$item] } else { "" })
            Context = $(Get-RoadmapIssueContext `
                -RoadmapPath $RoadmapPath `
                -ModuleName $ModuleName `
                -ItemTitle $item `
                -Target $(if ($targetByItem.ContainsKey($item)) { $targetByItem[$item] } else { "" }))
        }
    }
    
    if ($itemsToCreate.Count -eq 0) {
        Write-Host "  Keine neuen Issues zu erstellen!" -ForegroundColor Green
        Wait-Continue
        return
    }
    
    Write-Host "  Zu erstellen: $($itemsToCreate.Count) Issues" -ForegroundColor Cyan
    if ($preExistingUpdated -gt 0) {
        Write-Host "  ROADMAP bereits aktualisiert (existierende Issue/PR Treffer): $preExistingUpdated" -ForegroundColor Gray
    }
    if ($uncertainCount -gt 0) {
        Write-Host "  Mit [!] Markierung (unklare Treffer): $uncertainCount" -ForegroundColor Yellow
    }
    if ($UseAI) {
        Write-Host "  AI-Optimierung: AKTIV" -ForegroundColor Green
    }
    Write-Host ""
    
    if ($DryRun) {
        Write-Host "  DRY RUN MODE - Keine echten Issues werden erstellt" -ForegroundColor Yellow
        Write-Host ""
    }
    
    if (-not $AutoConfirm) {
        $choice = Get-UserChoice -Prompt "Fortfahren?" -Options @("Ja", "Nein") -Default 1

        if ($choice -ne 1) {
            Write-Host "  Abgebrochen." -ForegroundColor Yellow
            Wait-Continue
            return
        }
    } else {
        Write-Host "  Auto-Bestaetigung aktiv (-Yes)" -ForegroundColor Gray
    }
    
    Write-Host ""
    
    $created = 0
    $failed = 0
    $current = 0
    $issueLabels = @($ModuleName, "enhancement", "priority:medium")

    $labelsReady = Ensure-LabelsExist -Repo $Repository -Labels $issueLabels -DryRun:$DryRun
    if (-not $labelsReady) {
        Write-Host "" 
        Write-Host "  Abbruch: Labels konnten nicht vorbereitet werden." -ForegroundColor Red
        Wait-Continue
        return
    }
    
    foreach ($itemData in $itemsToCreate) {
        if ($MaxIssues -gt 0 -and $current -ge $MaxIssues) {
            Write-Host "  MaxIssues erreicht ($MaxIssues), stoppe weitere Erstellung." -ForegroundColor Gray
            break
        }

        $current++

        $item = $itemData.Text
        $isUncertain = $itemData.Uncertain
        $candidate = $itemData.Candidate
        $target = $itemData.Target
        $context = $itemData.Context
        
        Write-Host "  [$current/$($itemsToCreate.Count)] $item" -ForegroundColor White
        
        # AI-Optimierung
        if ($UseAI) {
            $optimizedTitle = Optimize-IssueTitle `
                -OriginalTitle $item `
                -ModuleName $ModuleName `
                -RoadmapMarker "[ ]" `
                -IsUncertain:$isUncertain
        } else {
            $optimizedTitle = $item
        }

        if ($isUncertain) {
            $optimizedTitle = "[!] $optimizedTitle"
        }
        
        $milestoneTitle = ""
        $milestoneNumber = 0
        if ($EnableMilestones) {
            $milestoneTitle = Resolve-MilestoneTitle -ModuleName $ModuleName -Target $target -MilestonePrefix $MilestonePrefix
            if (-not [string]::IsNullOrWhiteSpace($milestoneTitle)) {
                $milestoneNumber = Get-OrCreateMilestoneNumber -Repo $Repository -MilestoneTitle $milestoneTitle -DryRun:$DryRun
                if ($milestoneNumber -gt 0) {
                    Write-Host "    Milestone zugewiesen: $milestoneTitle (#$milestoneNumber)" -ForegroundColor Gray
                }
            }
        }

        $relationshipBlock = ""
        if ($EnableRelationships -and $candidate) {
            $candidateType = if ($candidate.WorkType) { $candidate.WorkType.ToUpper() } else { "WORK" }
            $candidateNumber = if ($candidate.number) { "#$($candidate.number)" } else { "" }
            $candidateUrl = if ($candidate.url) { "$($candidate.url)" } else { "" }

            $relationshipBlock = @"
**Related Work:**
- Potential relation to $candidateType $candidateNumber
$(if (-not [string]::IsNullOrWhiteSpace($candidateUrl)) { "- URL: $candidateUrl" } else { "" })

"@
        }

        $phaseBlock = ""
        if ($context -and $context.PhaseSteps -and $context.PhaseSteps.Count -gt 0) {
            $phaseLines = @($context.PhaseSteps | ForEach-Object { "- $_" })
            $phaseBlock = @"
**Implementation Phases:**
$($phaseLines -join "`n")

"@
        }

        $contextBlock = ""
        if ($context) {
            $contextLines = @()
            if (-not [string]::IsNullOrWhiteSpace($context.SectionTitle)) {
                $contextLines += "- Roadmap section: $($context.SectionTitle)"
            }
            if (-not [string]::IsNullOrWhiteSpace($context.Target)) {
                $contextLines += "- Target: $($context.Target)"
            }
            if (-not [string]::IsNullOrWhiteSpace($milestoneTitle)) {
                $contextLines += "- Planned milestone: $milestoneTitle"
            }
            if ($context.EnhancementHints -and $context.EnhancementHints.Count -gt 0) {
                $hintSample = @($context.EnhancementHints | Select-Object -First 3)
                foreach ($h in $hintSample) {
                    $contextLines += "- Enhancement hint: $h"
                }
            }

            if ($contextLines.Count -gt 0) {
                $contextBlock = @"
**Implementation Context:**
$($contextLines -join "`n")

"@
            }
        }

        $taskLines = @()
        if ($context -and $context.PhaseSteps -and $context.PhaseSteps.Count -gt 0) {
            $taskLines = @($context.PhaseSteps | ForEach-Object { "- [ ] $_" })
        } else {
            $taskLines = @(
                "- [ ] Design implementation architecture and interfaces",
                "- [ ] Implement production-ready functionality (no stubs)",
                "- [ ] Add robust error handling and input validation",
                "- [ ] Add or extend automated tests",
                "- [ ] Update module documentation and usage examples",
                "- [ ] Measure and optimize runtime performance"
            )
        }

        $acceptanceLines = @()
        if ($context -and $context.AcceptanceCriteria -and $context.AcceptanceCriteria.Count -gt 0) {
            $acceptanceLines = @($context.AcceptanceCriteria | ForEach-Object { "- $_" })
        } else {
            $acceptanceLines = @(
                "- Feature is fully functional and tested",
                "- Tests pass with >80% code coverage",
                "- Documentation is complete and accurate",
                "- No breaking changes to existing APIs",
                "- Performance meets requirements"
            )
        }

        $riskLines = @()
        if ($context -and $context.KnownLimitations) {
            $riskLines += @($context.KnownLimitations | ForEach-Object { "- $_" })
        }
        if ($context -and $context.BreakingChanges) {
            $riskLines += @($context.BreakingChanges | ForEach-Object { "- $_" })
        }

        $riskBlock = ""
        if ($riskLines.Count -gt 0) {
            $riskPreview = @($riskLines | Select-Object -First 8)
            $riskBlock = @"
**Constraints & Risks:**
    $($riskPreview -join "`n")

"@
        }

                $ruleRefs = Get-DesignRuleReferences -ModuleName $ModuleName
                $ruleRefLines = @()
                if ($ruleRefs.Count -gt 0) {
                        $ruleRefLines = @($ruleRefs | ForEach-Object { "  - $_" })
                } else {
                        $ruleRefLines = @("  - docs/ (module-specific design and implementation guides)")
                }

                $workflowBlock = @"
**Mandatory Delivery Workflow (ThemisDB Rules):**
- [ ] Phase 0: Existing code review before implementation
    - [ ] Identify existing files/symbols/interfaces and document reuse plan
    - [ ] Verify no duplicate implementation of existing functionality
    - [ ] Record affected files and integration points before coding
- [ ] Design and implementation rules reviewed from:
$($ruleRefLines -join "`n")
- [ ] Architecture and compatibility validation
    - [ ] Confirm behavior compatibility with existing APIs unless breaking change is explicitly declared
    - [ ] Confirm telemetry/logging/metrics integration follows existing module patterns
- [ ] Code review gate before completion
    - [ ] Self-review against roadmap acceptance criteria
    - [ ] Cross-check for overlap/duplication with existing implementations
- [ ] Validation gate
    - [ ] Unit + integration tests updated/added
    - [ ] Performance impact measured against baseline

"@
        
        # Generiere Body
        $body = @"
**Module:** $ModuleName

**Description:**
$item

$(if ($isUncertain) {
"**Uncertainty Note:**
Potential overlap with existing GitHub work item. Please verify manually before implementation.
"
} else { "" })

$contextBlock
$phaseBlock
$relationshipBlock
$workflowBlock

**Implementation Tasks:**
$($taskLines -join "`n")

**Acceptance Criteria:**
$($acceptanceLines -join "`n")

$riskBlock

**Generated by:** AI-powered GitHub Management Script
$(if ($UseAI) { "**AI Model:** $script:OllamaModel" } else { "" })
**Source Roadmap:** $RoadmapPath
"@

        if ($DryRun) {
            if ($EnableMilestones) {
                $dryMilestoneTitle = Resolve-MilestoneTitle -ModuleName $ModuleName -Target $target -MilestonePrefix $MilestonePrefix
                if (-not [string]::IsNullOrWhiteSpace($dryMilestoneTitle)) {
                    Write-Host "    [DRY RUN] Wuerde Milestone zuweisen: $dryMilestoneTitle" -ForegroundColor Yellow
                }
            }
            if ($EnableRelationships -and $candidate) {
                $candidateType = if ($candidate.WorkType) { $candidate.WorkType.ToUpper() } else { "WORK" }
                $candidateNumber = if ($candidate.number) { "#$($candidate.number)" } else { "" }
                Write-Host "    [DRY RUN] Wuerde Relationship setzen: $candidateType $candidateNumber" -ForegroundColor Yellow
            }
            Write-Host "    [DRY RUN] Wuerde Issue erstellen: [$ModuleName] $optimizedTitle" -ForegroundColor Yellow
            Write-Host "    [DRY RUN] Body enthält: Tasks=$($taskLines.Count), AC=$($acceptanceLines.Count), Risiken=$($riskLines.Count), RuleRefs=$($ruleRefs.Count)" -ForegroundColor DarkYellow
            Write-Host ""
            $created++
            Start-Sleep -Milliseconds 100
            continue
        }
        
        $suggestedLabels = $issueLabels
        
        $issueResult = New-GitHubIssue `
            -Repo $Repository `
            -Title "[$ModuleName] $optimizedTitle" `
            -Body $body `
            -Labels $suggestedLabels `
            -MilestoneNumber $milestoneNumber
        
        if ($issueResult.Success) {
            $created++

            $statusAfterCreate = if ($isUncertain) { "!" } else { "I" }
            $refTypeAfterCreate = if ($isUncertain) { "Issue" } else { "Issue" }
            $refNumberAfterCreate = if ($issueResult.Number) { [int]$issueResult.Number } else { 0 }

            $null = Set-RoadmapItemStatus `
                -RoadmapPath $RoadmapPath `
                -ItemTitle $item `
                -NewStatus $statusAfterCreate `
                -ReferenceType $refTypeAfterCreate `
                -ReferenceNumber $refNumberAfterCreate `
                -DryRun:$DryRun
        } else {
            $failed++

            $null = Set-RoadmapItemStatus `
                -RoadmapPath $RoadmapPath `
                -ItemTitle $item `
                -NewStatus "?" `
                -DryRun:$DryRun
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
        Exit-WithCode -Reason "Source directory not found" -Code $script:ExitCodes.SourceDirMissing
    }
    Write-Host " OK" -ForegroundColor Green
    Write-Host "        $script:SourceDir" -ForegroundColor Gray
    
    Write-Host "  [3/5] Pruefe GitHub CLI..." -NoNewline
    if (-not (Test-GitHubCLI)) {
        Write-Host " FEHLER" -ForegroundColor Red
        Write-Host ""
        Write-Host "  GitHub CLI nicht installiert!" -ForegroundColor Red
        Write-Host "  Download: https://cli.github.com/" -ForegroundColor Yellow
        Exit-WithCode -Reason "GitHub CLI missing" -Code $script:ExitCodes.GitHubCliMissing
    }
    Write-Host " OK" -ForegroundColor Green
    
    Write-Host "  [4/5] Pruefe GitHub Authentifizierung..." -NoNewline
    if (-not (Test-GitHubAuth)) {
        Write-Host " FEHLER" -ForegroundColor Red
        Write-Host ""
        Write-Host "  Nicht authentifiziert!" -ForegroundColor Red
        Write-Host "  Fuehren Sie aus: gh auth login" -ForegroundColor Yellow
        Exit-WithCode -Reason "GitHub authentication failed" -Code $script:ExitCodes.GitHubAuthFailed
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
                            -RoadmapPath $module.RoadmapPath `
                            -UseAI:$script:UseAI `
                            -DryRun:$DryRun `
                            -EnableMilestones:$EnableMilestones `
                            -EnableRelationships:$EnableRelationships `
                            -MilestonePrefix $MilestonePrefix `
                            -IncludeUnknownForCreation:$IncludeUnknownForCreation `
                            -OnlyItem $OnlyItem
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

function Start-Automation {
    Initialize-System

    if (-not $Yes) {
        Write-Host "  Hinweis: Automation ohne -Yes erkannt, setze Auto-Bestaetigung aktiv." -ForegroundColor Gray
    }
    $autoConfirm = $true

    switch ($Action) {
        "stats" {
            Show-RoadmapStatistics
            return
        }
        "create-module-issues" {
            $module = Get-ModuleByName -Name $ModuleName
            if (-not $module) {
                Write-Host "" -ForegroundColor Red
                Write-Host "  Modul nicht gefunden: '$ModuleName'" -ForegroundColor Red
                Write-Host "  Tipp: -Action create-all-issues oder gueltigen -ModuleName setzen" -ForegroundColor Yellow
                Exit-WithCode -Reason "Module not found" -Code $script:ExitCodes.ModuleNotFound
            }

            $roadmapData = Get-RoadmapContent -RoadmapPath $module.RoadmapPath
            if (-not $roadmapData) {
                Write-Host "  ROADMAP konnte nicht gelesen werden: $($module.RoadmapPath)" -ForegroundColor Red
                Exit-WithCode -Reason "Roadmap read failed" -Code $script:ExitCodes.RoadmapReadFailed
            }

            New-ModuleIssuesAI `
                -ModuleName $module.Name `
                -RoadmapData $roadmapData `
                -RoadmapPath $module.RoadmapPath `
                -UseAI:$script:UseAI `
                -DryRun:$DryRun `
                -AutoConfirm:$autoConfirm `
                -MaxIssues $MaxIssues `
                -EnableMilestones:$EnableMilestones `
                -EnableRelationships:$EnableRelationships `
                -MilestonePrefix $MilestonePrefix `
                -IncludeUnknownForCreation:$IncludeUnknownForCreation `
                -OnlyItem $OnlyItem
            return
        }
        "create-all-issues" {
            $modules = Get-ChildItem $script:SourceDir -Directory | Where-Object {
                Test-Path (Join-Path -Path $_.FullName -ChildPath "ROADMAP.md")
            } | Sort-Object Name

            if ($modules.Count -eq 0) {
                Write-Host "  Keine Module mit ROADMAP gefunden." -ForegroundColor Yellow
                return
            }

            foreach ($module in $modules) {
                $roadmapPath = Join-Path -Path $module.FullName -ChildPath "ROADMAP.md"
                $roadmapData = Get-RoadmapContent -RoadmapPath $roadmapPath
                if ($roadmapData) {
                    New-ModuleIssuesAI `
                        -ModuleName $module.Name `
                        -RoadmapData $roadmapData `
                        -RoadmapPath $roadmapPath `
                        -UseAI:$script:UseAI `
                        -DryRun:$DryRun `
                        -AutoConfirm:$autoConfirm `
                        -MaxIssues $MaxIssues `
                        -EnableMilestones:$EnableMilestones `
                        -EnableRelationships:$EnableRelationships `
                        -MilestonePrefix $MilestonePrefix `
                        -IncludeUnknownForCreation:$IncludeUnknownForCreation `
                        -OnlyItem $OnlyItem
                }
            }
            return
        }
        "ollama-settings" {
            Show-OllamaSettings
            return
        }
        default {
            Write-Host "  Unbekannte Action: $Action" -ForegroundColor Red
            Exit-WithCode -Reason "Unknown action" -Code $script:ExitCodes.UnknownAction
        }
    }
}

# ============================================================================
# ENTRY POINT
# ============================================================================

if ($Help) {
    Show-Usage
    exit $script:ExitCodes.Success
}

if ($Action -eq "interactive") {
    Start-Main
} else {
    Start-Automation
}