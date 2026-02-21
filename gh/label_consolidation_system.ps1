#Requires -Version 5.1

<#
.SYNOPSIS
    GitHub Label Konsolidierungs-System
.DESCRIPTION
    Analysiert, konsolidiert und bereinigt das Label-System
#>

param(
    [string]$Repository = "makr-code/ThemisDB"
)

# ============================================================================
# HILFSFUNKTIONEN
# ============================================================================

function Show-Banner {
    param([string]$Title)
    Clear-Host
    Write-Host "================================================================" -ForegroundColor Cyan
    Write-Host "  $Title" -ForegroundColor Cyan
    Write-Host "================================================================" -ForegroundColor Cyan
    Write-Host ""
}

function Get-CurrentLabels {
    param([string]$Repo)
    
    Write-Host "Lade vorhandene Labels von GitHub..." -ForegroundColor Yellow
    
    try {
        $json = gh api "repos/$Repo/labels" --paginate 2>&1
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "FEHLER beim Laden der Labels!" -ForegroundColor Red
            return @()
        }
        
        $labels = $json | ConvertFrom-Json
        
        return $labels | ForEach-Object {
            [PSCustomObject]@{
                Name = $_.name
                Color = $_.color
                Description = $_.description
                IssueCount = 0  # Wird spaeter geladen
            }
        }
    } catch {
        Write-Host "FEHLER: $_" -ForegroundColor Red
        return @()
    }
}

function Get-LabelUsageStats {
    param([string]$Repo, [array]$Labels)
    
    Write-Host "Analysiere Label-Verwendung..." -ForegroundColor Yellow
    
    $stats = @{}
    
    foreach ($label in $Labels) {
        Write-Host "  Pruefe: $($label.Name)..." -NoNewline
        
        try {
            $encodedLabel = [System.Web.HttpUtility]::UrlEncode($label.Name)
            $issues = gh api "repos/$Repo/issues?labels=$encodedLabel&state=all&per_page=1" 2>&1 | ConvertFrom-Json
            
            # GitHub API gibt total_count nicht immer zurueck, daher zaehlen wir
            $count = gh api "repos/$Repo/issues?labels=$encodedLabel&state=all" --paginate 2>&1 | ConvertFrom-Json | Measure-Object | Select-Object -ExpandProperty Count
            
            $stats[$label.Name] = $count
            Write-Host " $count Issues" -ForegroundColor Gray
        } catch {
            $stats[$label.Name] = 0
            Write-Host " 0 Issues" -ForegroundColor Gray
        }
        
        Start-Sleep -Milliseconds 100
    }
    
    return $stats
}

# ============================================================================
# EMPFOHLENES LABEL-SCHEMA
# ============================================================================

function Get-RecommendedLabelSchema {
    return @{
        # ========== TYPE LABELS ==========
        "Type" = @(
            @{ Name = "type:bug"; Color = "d73a4a"; Description = "Something isn't working"; Category = "Type" },
            @{ Name = "type:enhancement"; Color = "a2eeef"; Description = "New feature or improvement"; Category = "Type" },
            @{ Name = "type:documentation"; Color = "0075ca"; Description = "Documentation improvements"; Category = "Type" },
            @{ Name = "type:refactor"; Color = "fbca04"; Description = "Code refactoring"; Category = "Type" },
            @{ Name = "type:test"; Color = "1d76db"; Description = "Testing related"; Category = "Type" },
            @{ Name = "type:chore"; Color = "fef2c0"; Description = "Maintenance tasks"; Category = "Type" }
        )
        
        # ========== PRIORITY LABELS ==========
        "Priority" = @(
            @{ Name = "priority:critical"; Color = "b60205"; Description = "Critical priority - immediate action required"; Category = "Priority" },
            @{ Name = "priority:high"; Color = "d93f0b"; Description = "High priority"; Category = "Priority" },
            @{ Name = "priority:medium"; Color = "fbca04"; Description = "Medium priority"; Category = "Priority" },
            @{ Name = "priority:low"; Color = "0e8a16"; Description = "Low priority"; Category = "Priority" }
        )
        
        # ========== STATUS LABELS ==========
        "Status" = @(
            @{ Name = "status:planning"; Color = "d4c5f9"; Description = "In planning phase"; Category = "Status" },
            @{ Name = "status:in-progress"; Color = "ededed"; Description = "Currently being worked on"; Category = "Status" },
            @{ Name = "status:blocked"; Color = "e99695"; Description = "Blocked by dependency"; Category = "Status" },
            @{ Name = "status:review"; Color = "c5def5"; Description = "Ready for review"; Category = "Status" },
            @{ Name = "status:on-hold"; Color = "fef2c0"; Description = "On hold"; Category = "Status" }
        )
        
        # ========== AREA/MODULE LABELS ==========
        "Area" = @(
            @{ Name = "area:acceleration"; Color = "5319e7"; Description = "GPU acceleration module"; Category = "Area" },
            @{ Name = "area:analytics"; Color = "1d76db"; Description = "Analytics module"; Category = "Area" },
            @{ Name = "area:api"; Color = "0366d6"; Description = "API module"; Category = "Area" },
            @{ Name = "area:auth"; Color = "fbca04"; Description = "Authentication module"; Category = "Area" },
            @{ Name = "area:core"; Color = "d73a4a"; Description = "Core module"; Category = "Area" },
            @{ Name = "area:database"; Color = "c5def5"; Description = "Database/Storage"; Category = "Area" },
            @{ Name = "area:geo"; Color = "0e8a16"; Description = "Geospatial module"; Category = "Area" },
            @{ Name = "area:graph"; Color = "d876e3"; Description = "Graph module"; Category = "Area" },
            @{ Name = "area:llm"; Color = "e99695"; Description = "LLM integration"; Category = "Area" },
            @{ Name = "area:network"; Color = "bfdadc"; Description = "Network module"; Category = "Area" },
            @{ Name = "area:security"; Color = "b60205"; Description = "Security module"; Category = "Area" },
            @{ Name = "area:testing"; Color = "ededed"; Description = "Testing infrastructure"; Category = "Area" }
        )
        
        # ========== EFFORT LABELS ==========
        "Effort" = @(
            @{ Name = "effort:small"; Color = "c2e0c6"; Description = "Small effort (< 1 day)"; Category = "Effort" },
            @{ Name = "effort:medium"; Color = "fbca04"; Description = "Medium effort (1-3 days)"; Category = "Effort" },
            @{ Name = "effort:large"; Color = "d93f0b"; Description = "Large effort (> 3 days)"; Category = "Effort" },
            @{ Name = "effort:epic"; Color = "b60205"; Description = "Epic (> 1 week)"; Category = "Effort" }
        )
        
        # ========== SPECIAL LABELS ==========
        "Special" = @(
            @{ Name = "good-first-issue"; Color = "7057ff"; Description = "Good for newcomers"; Category = "Special" },
            @{ Name = "help-wanted"; Color = "008672"; Description = "Extra attention needed"; Category = "Special" },
            @{ Name = "production-blocker"; Color = "b60205"; Description = "Blocks production release"; Category = "Special" },
            @{ Name = "breaking-change"; Color = "d93f0b"; Description = "Breaking API change"; Category = "Special" },
            @{ Name = "security"; Color = "ee0701"; Description = "Security issue"; Category = "Special" },
            @{ Name = "performance"; Color = "fbca04"; Description = "Performance related"; Category = "Special" },
            @{ Name = "dependencies"; Color = "0366d6"; Description = "Dependency updates"; Category = "Special" }
        )
    }
}

# ============================================================================
# ANALYSE-FUNKTIONEN
# ============================================================================

function Show-LabelAnalysis {
    param([array]$CurrentLabels, [hashtable]$RecommendedSchema)
    
    Show-Banner "Label-Analyse"
    
    Write-Host "AKTUELLER STATUS:" -ForegroundColor Cyan
    Write-Host "  Vorhandene Labels: $($CurrentLabels.Count)" -ForegroundColor White
    Write-Host ""
    
    # Kategorisierung
    $categorized = @{
        "Konsistent" = @()
        "Aehnlich" = @()
        "Unbekannt" = @()
        "Deprecated" = @()
    }
    
    # Alle empfohlenen Labels sammeln
    $allRecommended = @()
    foreach ($category in $RecommendedSchema.Keys) {
        $allRecommended += $RecommendedSchema[$category]
    }
    
    foreach ($label in $CurrentLabels) {
        $found = $false
        
        # Exakte Uebereinstimmung
        if ($allRecommended.Name -contains $label.Name) {
            $categorized["Konsistent"] += $label
            $found = $true
        }
        # Aehnliche Labels
        elseif ($label.Name -match "^(type|priority|status|area|effort)[:_-]") {
            $categorized["Aehnlich"] += $label
            $found = $true
        }
        
        if (-not $found) {
            # Deprecated Labels
            $deprecatedPatterns = @("old-", "legacy-", "unused-", "deprecated")
            $isDeprecated = $false
            foreach ($pattern in $deprecatedPatterns) {
                if ($label.Name -like "*$pattern*") {
                    $categorized["Deprecated"] += $label
                    $isDeprecated = $true
                    break
                }
            }
            
            if (-not $isDeprecated) {
                $categorized["Unbekannt"] += $label
            }
        }
    }
    
    Write-Host "KATEGORISIERUNG:" -ForegroundColor Cyan
    Write-Host "  Konsistente Labels:  $($categorized['Konsistent'].Count)" -ForegroundColor Green
    Write-Host "  Aehnliche Labels:    $($categorized['Aehnlich'].Count)" -ForegroundColor Yellow
    Write-Host "  Unbekannte Labels:   $($categorized['Unbekannt'].Count)" -ForegroundColor Red
    Write-Host "  Deprecated Labels:   $($categorized['Deprecated'].Count)" -ForegroundColor Gray
    Write-Host ""
    
    # Details anzeigen
    if ($categorized['Aehnlich'].Count -gt 0) {
        Write-Host "AEHNLICHE LABELS (sollten konsolidiert werden):" -ForegroundColor Yellow
        foreach ($label in $categorized['Aehnlich']) {
            Write-Host "  - $($label.Name)" -ForegroundColor Yellow
        }
        Write-Host ""
    }
    
    if ($categorized['Unbekannt'].Count -gt 0) {
        Write-Host "UNBEKANNTE LABELS:" -ForegroundColor Red
        foreach ($label in $categorized['Unbekannt']) {
            Write-Host "  - $($label.Name)" -ForegroundColor Red
        }
        Write-Host ""
    }
    
    if ($categorized['Deprecated'].Count -gt 0) {
        Write-Host "DEPRECATED LABELS (sollten geloescht werden):" -ForegroundColor Gray
        foreach ($label in $categorized['Deprecated']) {
            Write-Host "  - $($label.Name)" -ForegroundColor Gray
        }
        Write-Host ""
    }
    
    return $categorized
}

function Show-MigrationPlan {
    param([array]$CurrentLabels, [hashtable]$RecommendedSchema)
    
    Show-Banner "Migrations-Plan"
    
    Write-Host "LABEL-MAPPING (Alt -> Neu):" -ForegroundColor Cyan
    Write-Host ""
    
    # Haeufige Mapping-Patterns
    $mappings = @{
        # Type Mappings
        "bug" = "type:bug"
        "enhancement" = "type:enhancement"
        "feature" = "type:enhancement"
        "documentation" = "type:documentation"
        "docs" = "type:documentation"
        "refactoring" = "type:refactor"
        "tests" = "type:test"
        "testing" = "type:test"
        
        # Priority Mappings
        "critical" = "priority:critical"
        "high" = "priority:high"
        "high-priority" = "priority:high"
        "medium" = "priority:medium"
        "low" = "priority:low"
        "low-priority" = "priority:low"
        
        # Status Mappings
        "in-progress" = "status:in-progress"
        "wip" = "status:in-progress"
        "work-in-progress" = "status:in-progress"
        "blocked" = "status:blocked"
        "on-hold" = "status:on-hold"
        "ready-for-review" = "status:review"
        "review" = "status:review"
        
        # Module Mappings
        "acceleration" = "area:acceleration"
        "cuda" = "area:acceleration"
        "vulkan" = "area:acceleration"
        "analytics" = "area:analytics"
        "api" = "area:api"
        "auth" = "area:auth"
        "authentication" = "area:auth"
        "core" = "area:core"
        "database" = "area:database"
        "storage" = "area:database"
        "geo" = "area:geo"
        "geospatial" = "area:geo"
        "graph" = "area:graph"
        "llm" = "area:llm"
        "security" = "area:security"
        
        # Special Mappings
        "production-blocker" = "production-blocker"
        "good-first-issue" = "good-first-issue"
        "help-wanted" = "help-wanted"
    }
    
    $migrationPlan = @()
    
    foreach ($label in $CurrentLabels) {
        $oldName = $label.Name
        $newName = $null
        
        # Direkte Mappings
        if ($mappings.ContainsKey($oldName)) {
            $newName = $mappings[$oldName]
        }
        # Bereits korrekt benannte Labels
        elseif ($oldName -match "^(type|priority|status|area|effort)[:_-]") {
            # Normalisieren (z.B. type_bug -> type:bug)
            $newName = $oldName -replace "[:_-]", ":"
        }
        
        if ($newName -and $newName -ne $oldName) {
            $migrationPlan += @{
                Old = $oldName
                New = $newName
                IssueCount = $label.IssueCount
            }
            
            Write-Host "  $oldName" -NoNewline -ForegroundColor Yellow
            Write-Host " -> " -NoNewline -ForegroundColor Gray
            Write-Host "$newName" -ForegroundColor Green
        }
    }
    
    Write-Host ""
    Write-Host "Gefunden: $($migrationPlan.Count) Labels zum Migrieren" -ForegroundColor Cyan
    Write-Host ""
    
    return $migrationPlan
}

# ============================================================================
# MIGRATIONS-FUNKTIONEN
# ============================================================================

function Invoke-LabelMigration {
    param([string]$Repo, [array]$MigrationPlan, [hashtable]$RecommendedSchema)
    
    Show-Banner "Label-Migration"
    
    Write-Host "WARNUNG: Dieser Vorgang wird Labels umbenennen und Issues aktualisieren!" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Druecken Sie Ctrl+C zum Abbrechen oder Enter zum Fortfahren..." -ForegroundColor Yellow
    Read-Host | Out-Null
    
    Write-Host ""
    Write-Host "Starte Migration..." -ForegroundColor Cyan
    Write-Host ""
    
    # Schritt 1: Neue Labels erstellen
    Write-Host "[1/4] Erstelle neue Labels..." -ForegroundColor Yellow
    
    foreach ($category in $RecommendedSchema.Keys) {
        foreach ($label in $RecommendedSchema[$category]) {
            Write-Host "  Creating: $($label.Name)..." -NoNewline
            
            try {
                gh api repos/$Repo/labels `
                    -f name="$($label.Name)" `
                    -f color="$($label.Color)" `
                    -f description="$($label.Description)" 2>&1 | Out-Null
                
                if ($LASTEXITCODE -eq 0) {
                    Write-Host " OK" -ForegroundColor Green
                } else {
                    Write-Host " (existiert bereits)" -ForegroundColor Gray
                }
            } catch {
                Write-Host " FEHLER" -ForegroundColor Red
            }
            
            Start-Sleep -Milliseconds 100
        }
    }
    
    Write-Host ""
    
    # Schritt 2: Issues migrieren
    Write-Host "[2/4] Migriere Issues zu neuen Labels..." -ForegroundColor Yellow
    
    foreach ($mapping in $MigrationPlan) {
        Write-Host "  Migriere '$($mapping.Old)' -> '$($mapping.New)'..." -NoNewline
        
        try {
            # Finde alle Issues mit altem Label
            $encodedOld = [System.Web.HttpUtility]::UrlEncode($mapping.Old)
            $issues = gh api "repos/$Repo/issues?labels=$encodedOld&state=all" --paginate 2>&1 | ConvertFrom-Json
            
            if ($issues.Count -gt 0) {
                foreach ($issue in $issues) {
                    $issueNumber = $issue.number
                    
                    # Fuege neues Label hinzu
                    gh api "repos/$Repo/issues/$issueNumber/labels" `
                        -f labels[]="$($mapping.New)" 2>&1 | Out-Null
                    
                    # Entferne altes Label
                    $encodedOldForDelete = [System.Web.HttpUtility]::UrlEncode($mapping.Old)
                    gh api "repos/$Repo/issues/$issueNumber/labels/$encodedOldForDelete" `
                        -X DELETE 2>&1 | Out-Null
                    
                    Start-Sleep -Milliseconds 100
                }
                
                Write-Host " $($issues.Count) Issues" -ForegroundColor Green
            } else {
                Write-Host " keine Issues" -ForegroundColor Gray
            }
        } catch {
            Write-Host " FEHLER: $_" -ForegroundColor Red
        }
    }
    
    Write-Host ""
    
    # Schritt 3: Alte Labels loeschen
    Write-Host "[3/4] Loesche alte Labels..." -ForegroundColor Yellow
    Write-Host "  Moechten Sie alte Labels jetzt loeschen? (j/n): " -NoNewline
    $confirm = Read-Host
    
    if ($confirm -eq "j" -or $confirm -eq "J") {
        foreach ($mapping in $MigrationPlan) {
            Write-Host "  Deleting: $($mapping.Old)..." -NoNewline
            
            try {
                $encodedOld = [System.Web.HttpUtility]::UrlEncode($mapping.Old)
                gh api "repos/$Repo/labels/$encodedOld" -X DELETE 2>&1 | Out-Null
                
                if ($LASTEXITCODE -eq 0) {
                    Write-Host " OK" -ForegroundColor Green
                } else {
                    Write-Host " FEHLER" -ForegroundColor Red
                }
            } catch {
                Write-Host " FEHLER" -ForegroundColor Red
            }
            
            Start-Sleep -Milliseconds 100
        }
    } else {
        Write-Host "  Uebersprungen" -ForegroundColor Yellow
    }
    
    Write-Host ""
    
    # Schritt 4: Zusammenfassung
    Write-Host "[4/4] Migration abgeschlossen!" -ForegroundColor Green
    Write-Host ""
}

# ============================================================================
# EXPORT-FUNKTIONEN
# ============================================================================

function Export-LabelSchemaToFile {
    param([hashtable]$Schema, [string]$OutputPath = "label_schema.json")
    
    $export = @{
        generated = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        repository = $Repository
        categories = $Schema
    }
    
    $export | ConvertTo-Json -Depth 10 | Out-File $OutputPath -Encoding UTF8
    
    Write-Host "Label-Schema exportiert nach: $OutputPath" -ForegroundColor Green
}

# ============================================================================
# HAUPTMENUE
# ============================================================================

function Show-MainMenu {
    Show-Banner "Label Consolidation System"
    
    Write-Host "OPTIONEN:" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  [1] Aktuelle Labels analysieren" -ForegroundColor White
    Write-Host "  [2] Empfohlenes Schema anzeigen" -ForegroundColor White
    Write-Host "  [3] Migrations-Plan erstellen" -ForegroundColor White
    Write-Host "  [4] Label-Migration durchfuehren" -ForegroundColor Yellow
    Write-Host "  [5] Ungenutzte Labels finden" -ForegroundColor White
    Write-Host "  [6] Schema als JSON exportieren" -ForegroundColor White
    Write-Host "  [7] Beenden" -ForegroundColor Red
    Write-Host ""
    
    Write-Host "Ihre Wahl [1-7]: " -NoNewline -ForegroundColor Cyan
    $choice = Read-Host
    
    return $choice
}

# ============================================================================
# HAUPTPROGRAMM
# ============================================================================

Show-Banner "Label Consolidation System"

Write-Host "Repository: $Repository" -ForegroundColor Cyan
Write-Host ""
Write-Host "Ueberpruefe GitHub CLI..." -NoNewline -ForegroundColor Yellow

try {
    gh --version 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host " OK" -ForegroundColor Green
    } else {
        throw "GitHub CLI nicht gefunden"
    }
} catch {
    Write-Host " FEHLER" -ForegroundColor Red
    Write-Host "Bitte installieren Sie GitHub CLI: https://cli.github.com/" -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "Druecken Sie Enter um fortzufahren..."
Read-Host | Out-Null

# Hauptschleife
$currentLabels = $null
$recommendedSchema = Get-RecommendedLabelSchema
$migrationPlan = $null

while ($true) {
    $choice = Show-MainMenu
    
    switch ($choice) {
        "1" {
            $currentLabels = Get-CurrentLabels -Repo $Repository
            if ($currentLabels.Count -gt 0) {
                $categorized = Show-LabelAnalysis -CurrentLabels $currentLabels -RecommendedSchema $recommendedSchema
                Write-Host ""
                Write-Host "Druecken Sie Enter um fortzufahren..."
                Read-Host | Out-Null
            }
        }
        
        "2" {
            Show-Banner "Empfohlenes Label-Schema"
            
            foreach ($category in $recommendedSchema.Keys | Sort-Object) {
                Write-Host "$category Labels:" -ForegroundColor Yellow
                foreach ($label in $recommendedSchema[$category]) {
                    Write-Host "  [$($label.Color)] $($label.Name.PadRight(30)) - $($label.Description)" -ForegroundColor White
                }
                Write-Host ""
            }
            
            $totalLabels = ($recommendedSchema.Values | ForEach-Object { $_.Count } | Measure-Object -Sum).Sum
            Write-Host "Gesamt: $totalLabels empfohlene Labels" -ForegroundColor Cyan
            Write-Host ""
            Write-Host "Druecken Sie Enter um fortzufahren..."
            Read-Host | Out-Null
        }
        
        "3" {
            if ($null -eq $currentLabels) {
                $currentLabels = Get-CurrentLabels -Repo $Repository
            }
            
            $migrationPlan = Show-MigrationPlan -CurrentLabels $currentLabels -RecommendedSchema $recommendedSchema
            Write-Host "Druecken Sie Enter um fortzufahren..."
            Read-Host | Out-Null
        }
        
        "4" {
            if ($null -eq $currentLabels) {
                $currentLabels = Get-CurrentLabels -Repo $Repository
            }
            
            if ($null -eq $migrationPlan) {
                $migrationPlan = Show-MigrationPlan -CurrentLabels $currentLabels -RecommendedSchema $recommendedSchema
            }
            
            Invoke-LabelMigration -Repo $Repository -MigrationPlan $migrationPlan -RecommendedSchema $recommendedSchema
            
            Write-Host "Druecken Sie Enter um fortzufahren..."
            Read-Host | Out-Null
        }
        
        "5" {
            if ($null -eq $currentLabels) {
                $currentLabels = Get-CurrentLabels -Repo $Repository
            }
            
            Show-Banner "Ungenutzte Labels"
            
            $stats = Get-LabelUsageStats -Repo $Repository -Labels $currentLabels
            $unused = $stats.GetEnumerator() | Where-Object { $_.Value -eq 0 } | Sort-Object Name
            
            Write-Host ""
            Write-Host "UNGENUTZTE LABELS (0 Issues):" -ForegroundColor Yellow
            Write-Host ""
            
            if ($unused.Count -eq 0) {
                Write-Host "  Keine ungenutzten Labels gefunden!" -ForegroundColor Green
            } else {
                foreach ($label in $unused) {
                    Write-Host "  - $($label.Name)" -ForegroundColor Red
                }
                
                Write-Host ""
                Write-Host "Gesamt: $($unused.Count) ungenutzte Labels" -ForegroundColor Cyan
            }
            
            Write-Host ""
            Write-Host "Druecken Sie Enter um fortzufahren..."
            Read-Host | Out-Null
        }
        
        "6" {
            Export-LabelSchemaToFile -Schema $recommendedSchema
            Write-Host ""
            Write-Host "Druecken Sie Enter um fortzufahren..."
            Read-Host | Out-Null
        }
        
        "7" {
            Write-Host ""
            Write-Host "Auf Wiedersehen!" -ForegroundColor Green
            exit
        }
        
        default {
            Write-Host "Ungueltige Eingabe!" -ForegroundColor Red
            Start-Sleep -Seconds 1
        }
    }
}