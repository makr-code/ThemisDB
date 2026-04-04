#Requires -Version 5.1

<#
.SYNOPSIS
    ThemisDB Milestone-Bereinigung
.DESCRIPTION
    Entfernt Modul-Präfixe aus GitHub-Milestone-Titeln.

    Unerwünschtes Muster : "<modul> - <version>"  z.B. "query - Q2 2026"
    Gewünschtes Muster   : "<version>"             z.B. "Q2 2026"

    Logik:
    1) Alle offenen Milestones laden.
    2) Modulabhängige Milestones anhand des Musters "<modul> - <version>" identifizieren.
    3a) Nur ein Präfix-Milestone für eine Version  →  direkt umbenennen.
    3b) Mehrere Präfix-Milestones für eine Version →  Issues in den kanonischen
        Milestone verschieben, Duplikate löschen.
    4) Vorhandene saubere Milestones werden als Ziel wiederverwendet.
.PARAMETER Repository
    GitHub-Repository im Format "owner/repo" (Standard: makr-code/ThemisDB).
.PARAMETER DryRun
    Zeigt nur, was getan werden würde – führt keine Änderungen durch.
.EXAMPLE
    .\cleanup_milestones.ps1
    .\cleanup_milestones.ps1 -DryRun
    .\cleanup_milestones.ps1 -Repository "myorg/myrepo"
#>

param(
    # Default matches the ThemisDB repository – consistent with all other scripts in this folder.
    [string]$Repository = "makr-code/ThemisDB",
    [switch]$DryRun
)

# ============================================================================
# HILFSFUNKTIONEN
# ============================================================================

function Invoke-GhApi {
    <#
    .SYNOPSIS
        Führt einen gh-API-Aufruf aus und gibt das deserialisierte JSON-Objekt zurück.
    .OUTPUTS
        Deserialisiertes JSON-Objekt, leeres PSCustomObject bei erfolgreichem Aufruf
        ohne Antwort-Body (z.B. HTTP 204), oder $null bei Fehler.
    #>
    param(
        [string]$Endpoint,
        [string]$Method = "GET",
        [hashtable]$Fields = @{}
    )

    $ghArgs = @("api", $Endpoint, "-X", $Method)
    foreach ($key in $Fields.Keys) {
        $ghArgs += "-f"
        $ghArgs += "$key=$($Fields[$key])"
    }

    $output = & gh @ghArgs 2>&1
    if ($LASTEXITCODE -ne 0) {
        return $null
    }

    if ([string]::IsNullOrWhiteSpace($output)) {
        # Erfolgreicher Aufruf ohne Antwort-Body (z.B. HTTP 204 DELETE)
        return [PSCustomObject]@{}
    }

    try {
        return $output | ConvertFrom-Json
    } catch {
        return $null
    }
}

function Get-OpenMilestones {
    <#
    .SYNOPSIS
        Lädt alle offenen Milestones (paginiert) aus dem Repository.
    #>
    param([string]$Repo)

    Write-Host "Lade offene Milestones..." -ForegroundColor Yellow

    $all = [System.Collections.Generic.List[object]]::new()
    $page = 1

    do {
        $batch = Invoke-GhApi -Endpoint "repos/$Repo/milestones?state=open&per_page=100&page=$page"
        if ($null -eq $batch) {
            Write-Host "  FEHLER beim Laden der Milestones!" -ForegroundColor Red
            exit 1
        }
        foreach ($ms in $batch) { $all.Add($ms) }
        $page++
    } while ($batch.Count -eq 100)

    Write-Host "  $($all.Count) offene Milestones gefunden." -ForegroundColor Gray
    Write-Host ""

    return $all.ToArray()
}

function Get-IssuesForMilestone {
    <#
    .SYNOPSIS
        Gibt alle Issues (offen + geschlossen) für einen Milestone zurück.
    #>
    param([string]$Repo, [int]$MilestoneNumber)

    $issues = [System.Collections.Generic.List[object]]::new()

    foreach ($state in @("open", "closed")) {
        $page = 1
        do {
            $batch = Invoke-GhApi -Endpoint "repos/$Repo/issues?milestone=$MilestoneNumber&state=$state&per_page=100&page=$page"
            if ($null -eq $batch) { break }
            foreach ($issue in $batch) { $issues.Add($issue) }
            $page++
        } while ($batch.Count -eq 100)
    }

    return $issues.ToArray()
}

function Set-IssueMilestone {
    <#
    .SYNOPSIS
        Verschiebt ein Issue zu einem anderen Milestone.
    #>
    param([string]$Repo, [int]$IssueNumber, [int]$MilestoneNumber)

    $result = Invoke-GhApi -Endpoint "repos/$Repo/issues/$IssueNumber" `
                           -Method "PATCH" `
                           -Fields @{ milestone = $MilestoneNumber }
    return ($null -ne $result)
}

function Rename-Milestone {
    <#
    .SYNOPSIS
        Benennt einen Milestone um.
    #>
    param([string]$Repo, [int]$MilestoneNumber, [string]$NewTitle)

    $result = Invoke-GhApi -Endpoint "repos/$Repo/milestones/$MilestoneNumber" `
                           -Method "PATCH" `
                           -Fields @{ title = $NewTitle }
    return ($null -ne $result)
}

function New-Milestone {
    <#
    .SYNOPSIS
        Erstellt einen neuen offenen Milestone und gibt das API-Objekt zurück.
    #>
    param([string]$Repo, [string]$Title)

    $ms = Invoke-GhApi -Endpoint "repos/$Repo/milestones" -Method "POST" -Fields @{ title = $Title }
    return $ms
}

function Remove-Milestone {
    <#
    .SYNOPSIS
        Löscht einen Milestone.
    #>
    param([string]$Repo, [int]$MilestoneNumber)

    $result = Invoke-GhApi -Endpoint "repos/$Repo/milestones/$MilestoneNumber" -Method "DELETE"
    return ($null -ne $result)
}

# ============================================================================
# HAUPTPROGRAMM
# ============================================================================

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  ThemisDB Milestone-Bereinigung" -ForegroundColor Cyan
if ($DryRun) {
    Write-Host "  [DRY-RUN – keine Änderungen werden durchgeführt]" -ForegroundColor Yellow
}
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Repository: $Repository" -ForegroundColor White
Write-Host ""

# Regex: "query - Q2 2026"  ->  Capture group 1 = "Q2 2026"
# Module name: one or more words (no spaces or dashes), separated by spaces.
# Separator: exactly " - " (space-dash-space).
$ModulePrefixRegex = [regex]'^[^\s-]+(?: [^\s-]+)* - (.+)$'

# -- 1) Milestones laden --------------------------------------------------

$milestones = Get-OpenMilestones -Repo $Repository

# -- 2) Milestones klassifizieren -----------------------------------------

$clean    = @{}   # version-title -> milestone object (bereits sauber benannt)
$prefixed = [System.Collections.Generic.List[object]]::new()   # modulabhängig

foreach ($ms in $milestones) {
    if ($ModulePrefixRegex.IsMatch($ms.title)) {
        $prefixed.Add($ms)
    } else {
        $clean[$ms.title] = $ms
    }
}

if ($prefixed.Count -eq 0) {
    Write-Host "Keine modulabhängigen Milestones gefunden. Nichts zu tun." -ForegroundColor Green
    Write-Host ""
    exit 0
}

Write-Host "Modulabhängige Milestones gefunden: $($prefixed.Count)" -ForegroundColor Yellow
foreach ($ms in $prefixed) {
    Write-Host "   - ""$($ms.title)""  (#$($ms.number))" -ForegroundColor White
}
Write-Host ""

# -- 3) Nach Ziel-Version gruppieren --------------------------------------

$byVersion = @{}
foreach ($ms in $prefixed) {
    $versionTitle = $ModulePrefixRegex.Match($ms.title).Groups[1].Value
    if (-not $byVersion.ContainsKey($versionTitle)) {
        $byVersion[$versionTitle] = [System.Collections.Generic.List[object]]::new()
    }
    $byVersion[$versionTitle].Add($ms)
}

$renamedCount    = 0
$deletedCount    = 0
$movedIssueCount = 0

# -- 4) Bereinigung je Ziel-Version ---------------------------------------

foreach ($versionTitle in $byVersion.Keys) {
    $group = $byVersion[$versionTitle]

    Write-Host "--- Ziel-Milestone: ""$versionTitle"" ---" -ForegroundColor Cyan

    if ($clean.ContainsKey($versionTitle)) {
        # Sauberer Milestone existiert bereits -> als Ziel verwenden
        $target = $clean[$versionTitle]
        Write-Host "  Bestehender Milestone gefunden: #$($target.number)" -ForegroundColor Gray
    } elseif ($group.Count -eq 1) {
        # Nur ein Präfix-Milestone -> direkt umbenennen
        $ms = $group[0]
        Write-Host "  Benenne um: ""$($ms.title)"" -> ""$versionTitle""" -ForegroundColor White

        if (-not $DryRun) {
            if (Rename-Milestone -Repo $Repository -MilestoneNumber $ms.number -NewTitle $versionTitle) {
                Write-Host "  Umbenannt." -ForegroundColor Green
                $renamedCount++
            } else {
                Write-Host "  FEHLER beim Umbenennen!" -ForegroundColor Red
            }
        } else {
            Write-Host "  [DRY-RUN] Würde umbenennen." -ForegroundColor DarkYellow
        }

        Write-Host ""
        continue
    } else {
        # Mehrere Präfix-Milestones -> neuen kanonischen Milestone erstellen
        Write-Host "  Erstelle neuen Milestone: ""$versionTitle""" -ForegroundColor White

        if (-not $DryRun) {
            $newMs = New-Milestone -Repo $Repository -Title $versionTitle
            if ($null -eq $newMs) {
                Write-Host "  FEHLER beim Erstellen – überspringe ""$versionTitle""." -ForegroundColor Red
                Write-Host ""
                continue
            }
            $target = $newMs
            $clean[$versionTitle] = $target
            Write-Host "  Erstellt: #$($target.number)" -ForegroundColor Green
        } else {
            Write-Host "  [DRY-RUN] Würde neuen Milestone erstellen." -ForegroundColor DarkYellow
            $target = [PSCustomObject]@{ number = 0 }
        }
    }

    # Issues verschieben und Präfix-Milestones löschen
    foreach ($ms in $group) {
        $issues = Get-IssuesForMilestone -Repo $Repository -MilestoneNumber $ms.number
        Write-Host "  Verschiebe $($issues.Count) Issues von ""$($ms.title)"" (#$($ms.number)) -> ""$versionTitle"" (#$($target.number))" -ForegroundColor White

        if (-not $DryRun) {
            foreach ($issue in $issues) {
                if (Set-IssueMilestone -Repo $Repository -IssueNumber $issue.number -MilestoneNumber $target.number) {
                    $movedIssueCount++
                } else {
                    Write-Host "    FEHLER beim Verschieben von Issue #$($issue.number)!" -ForegroundColor Red
                }
            }
        } else {
            Write-Host "  [DRY-RUN] Würde $($issues.Count) Issues verschieben." -ForegroundColor DarkYellow
        }

        Write-Host "  Lösche ""$($ms.title)"" (#$($ms.number))..." -ForegroundColor White

        if (-not $DryRun) {
            if (Remove-Milestone -Repo $Repository -MilestoneNumber $ms.number) {
                Write-Host "  Gelöscht." -ForegroundColor Green
                $deletedCount++
            } else {
                Write-Host "  FEHLER beim Löschen!" -ForegroundColor Red
            }
        } else {
            Write-Host "  [DRY-RUN] Würde Milestone löschen." -ForegroundColor DarkYellow
        }
    }

    Write-Host ""
}

# -- 5) Zusammenfassung ---------------------------------------------------

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Zusammenfassung" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
if ($DryRun) {
    Write-Host "  [DRY-RUN – keine Änderungen wurden durchgeführt]" -ForegroundColor Yellow
} else {
    Write-Host "  Umbenannte Milestones : $renamedCount" -ForegroundColor White
    Write-Host "  Gelöschte Milestones  : $deletedCount" -ForegroundColor White
    Write-Host "  Verschobene Issues    : $movedIssueCount" -ForegroundColor White
}
Write-Host ""
