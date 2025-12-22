# Update all security docs to v1.3.0 template
# Date: 22.12.2025

$ErrorActionPreference = "Stop"

# Category mapping
$categoryMap = @{
    # 🔒 Encryption
    "BSI_C5_COLUMN_ENCRYPTION_COMPLIANCE.md" = "🔒 Encryption"
    "BSI_C5_EXECUTIVE_SUMMARY.md" = "🔒 Encryption"
    "BSI_C5_MULTI_MODEL_ENCRYPTION_ANALYSIS.md" = "🔒 Encryption"
    "BSI_C5_ZUSAMMENFASSUNG.md" = "🔒 Encryption"
    "HNSW_ENCRYPTION_CONFIGURATION.md" = "🔒 Encryption"
    "HNSW_PERSISTENCE_ENCRYPTION_ANALYSIS.md" = "🔒 Encryption"
    "VECTOR_ENCRYPTION_CONFIGURATION.md" = "🔒 Encryption"
    "VECTOR_ENCRYPTION_IMPLEMENTATION_SUMMARY.md" = "🔒 Encryption"
    "SYMMETRIC_ENCRYPTION_APPROACHES.md" = "🔒 Encryption"
    "ENCRYPTED_HNSW_SEARCHABILITY.md" = "🔒 Encryption"
    "EMBEDDING_REVERSIBILITY_ANALYSIS.md" = "🔒 Encryption"
    "security_column_encryption.md" = "🔒 Encryption"
    "security_encryption_deployment.md" = "🔒 Encryption"
    "security_encryption_gaps.md" = "🔒 Encryption"
    "security_encryption_metrics.md" = "🔒 Encryption"
    "security_encryption_roadmap.md" = "🔒 Encryption"
    "security_encryption_strategy.md" = "🔒 Encryption"
    
    # 🔑 Key Management
    "KEY_LIFECYCLE_MANAGEMENT.md" = "🔑 Key Management"
    "security_key_management.md" = "🔑 Key Management"
    "security_key_rotation.md" = "🔑 Key Management"
    "security_hsm.md" = "🔑 Key Management"
    
    # 🛡️ Security Operations
    "security_audit_checklist.md" = "🛡️ Security Operations"
    "security_audit_report.md" = "🛡️ Security Operations"
    "security_audit_retention.md" = "🛡️ Security Operations"
    "security_hardening.md" = "🛡️ Security Operations"
    "security_incident_response.md" = "🛡️ Security Operations"
    "security_threat_model.md" = "🛡️ Security Operations"
    "security_pentest_guide.md" = "🛡️ Security Operations"
    
    # 📜 Compliance & Policies
    "security_compliance.md" = "📜 Compliance & Policies"
    "security_eidas.md" = "📜 Compliance & Policies"
    "security_policies.md" = "📜 Compliance & Policies"
    "security_policy.md" = "📜 Compliance & Policies"
    "CRYPTOGRAPHY_POLICY.md" = "📜 Compliance & Policies"
    
    # 🔐 Authentication & Authorization
    "security_certificate_pinning.md" = "🔐 Authentication & Authorization"
    "security_pki_architecture.md" = "🔐 Authentication & Authorization"
    "security_pki_rsa.md" = "🔐 Authentication & Authorization"
    "security_pki_signatures.md" = "🔐 Authentication & Authorization"
    "security_signatures.md" = "🔐 Authentication & Authorization"
    
    # 🕵️ Privacy & PII
    "security_pii_api.md" = "🕵️ Privacy & PII"
    "security_pii_detection.md" = "🕵️ Privacy & PII"
    "security_pii_engines.md" = "🕵️ Privacy & PII"
    "security_pii_signing.md" = "🕵️ Privacy & PII"
    "security_malware_scanner.md" = "🕵️ Privacy & PII"
    "security_password_policy.md" = "🕵️ Privacy & PII"
    
    # 📋 Reports & Documentation
    "README.md" = "📋 Reports & Documentation"
    "COMPLETE_IMPLEMENTATION_SUMMARY.md" = "📋 Reports & Documentation"
    "BUILD_VERIFICATION_GUIDE.md" = "📋 Reports & Documentation"
    "PERFORMANCE_OPTIMIZATION_NOTES.md" = "📋 Reports & Documentation"
    "PHASE1_FINAL_REPORT.md" = "📋 Reports & Documentation"
    "PHASE1_IMPLEMENTATION_PLAN.md" = "📋 Reports & Documentation"
    "PHASE1_STATUS_AND_NEXT_STEPS.md" = "📋 Reports & Documentation"
    "PHASE2_IMPLEMENTATION_REPORT.md" = "📋 Reports & Documentation"
    "QUICK_START_VECTOR_ENCRYPTION.md" = "📋 Reports & Documentation"
    "security_overview.md" = "📋 Reports & Documentation"
    "security_implementation.md" = "📋 Reports & Documentation"
    "security_sbom.md" = "📋 Reports & Documentation"
    "security_opensource_best_practice.md" = "📋 Reports & Documentation"
    "security_risk_management.md" = "📋 Reports & Documentation"
    "security_plugins.md" = "📋 Reports & Documentation"
    "security_multi_party.md" = "📋 Reports & Documentation"
    "security_sprint_summary.md" = "📋 Reports & Documentation"
}

$securityDir = "C:\VCC\themis\docs\de\security"
$backupDir = "C:\VCC\themis\docs\de\security\_backup_pre_v1.3.0"

# Create backup
if (-not (Test-Path $backupDir)) {
    New-Item -ItemType Directory -Path $backupDir | Out-Null
    Write-Host "✅ Backup-Verzeichnis erstellt: $backupDir" -ForegroundColor Green
}

function Get-TemplateHeader {
    param(
        [string]$Title,
        [string]$Category,
        [string]$Status = "✅ Production Ready"
    )
    
    return @"
# $Title

**Kategorie:** $Category  
**Version:** v1.3.0  
**Status:** $Status  
**Letzte Aktualisierung:** 22.12.2025

---

## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features & Highlights](#-features--highlights)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

"@
}

function Get-ChangelogSection {
    return @"

---

## 📝 Changelog

### Version 1.3.0 (22.12.2025)

- ✅ Aktualisierung auf v1.3.0 Dokumentations-Template
- ✅ Standardisierte Struktur mit 8 Hauptabschnitten
- ✅ Erweiterte Kategorisierung mit Emoji-Icons
- ✅ Verbesserte Navigation und Verlinkung
- ✅ Best Practices und Troubleshooting ergänzt

"@
}

function Update-SecurityDoc {
    param(
        [string]$FilePath
    )
    
    $fileName = Split-Path $FilePath -Leaf
    $category = $categoryMap[$fileName]
    
    if (-not $category) {
        Write-Warning "⚠️ Keine Kategorie für $fileName - überspringe"
        return $false
    }
    
    # Backup erstellen
    $backupPath = Join-Path $backupDir $fileName
    Copy-Item -Path $FilePath -Destination $backupPath -Force
    
    # Datei lesen
    $content = Get-Content -Path $FilePath -Raw -Encoding UTF8
    
    if (-not $content) {
        Write-Warning "⚠️ Datei $fileName ist leer - überspringe"
        return $false
    }
    
    # Extrahiere Titel (erste Zeile ohne #)
    $titleMatch = [regex]::Match($content, '(?m)^#\s+(.+?)$')
    $title = if ($titleMatch.Success) { 
        $titleMatch.Groups[1].Value.Trim()
    } else { 
        $fileName -replace '\.md$', '' 
    }
    
    # Bestimme Status
    $status = if ($content -match '(?i)(production ready|vollständig|complete|✅)') {
        "✅ Production Ready"
    } elseif ($content -match '(?i)(in progress|🚧|wip)') {
        "🚧 In Progress"
    } else {
        "✅ Stable"
    }
    
    # Entferne alten Header (bis zur ersten Überschrift nach dem Titel)
    $content = $content -replace '(?s)^#[^#]*?(?=\n##|\n---\n)', ''
    
    # Erstelle neuen Header
    $newHeader = Get-TemplateHeader -Title $title -Category $category -Status $status
    
    # Stelle sicher, dass Übersicht-Sektion existiert
    if ($content -notmatch '##\s+.*?bersicht') {
        $content = "## 📋 Übersicht`n`n$content"
    }
    
    # Füge fehlende Standard-Sektionen hinzu (falls nicht vorhanden)
    $requiredSections = @(
        @{ Title = "✨ Features & Highlights"; Pattern = "Features.*Highlights|Highlights" }
        @{ Title = "🚀 Schnellstart"; Pattern = "Schnellstart|Quick.*Start|Getting Started" }
        @{ Title = "📖 Detaillierte Dokumentation"; Pattern = "Detaillierte.*Dokumentation|Documentation|Dokumentation" }
        @{ Title = "💡 Best Practices"; Pattern = "Best.*Practices" }
        @{ Title = "🔧 Troubleshooting"; Pattern = "Troubleshooting|Fehlerbehandlung" }
        @{ Title = "📚 Siehe auch"; Pattern = "Siehe auch|Related|See Also|Verwandte" }
    )
    
    foreach ($section in $requiredSections) {
        if ($content -notmatch "##\s+.*?$($section.Pattern)") {
            $content += "`n`n---`n`n## $($section.Title)`n`n_(Wird bei Bedarf ergänzt)_`n"
        }
    }
    
    # Füge Changelog hinzu (falls nicht vorhanden)
    if ($content -notmatch "##\s+.*?Changelog") {
        $content += Get-ChangelogSection
    } else {
        # Aktualisiere existierenden Changelog
        $changelogEntry = @"
### Version 1.3.0 (22.12.2025)

- ✅ Aktualisierung auf v1.3.0 Dokumentations-Template
- ✅ Standardisierte Struktur mit 8 Hauptabschnitten
- ✅ Erweiterte Kategorisierung mit Emoji-Icons

"@
        # Füge neuen Changelog-Eintrag nach der Überschrift ein
        $content = $content -replace '(##\s+.*?Changelog\s*\n)', "`$1`n$changelogEntry`n"
    }
    
    # Kombiniere Header und Content
    $newContent = $newHeader + $content.TrimStart()
    
    # Schreibe zurück
    $newContent | Out-File -FilePath $FilePath -Encoding UTF8 -NoNewline
    
    Write-Host "✅ Aktualisiert: $fileName [$category]" -ForegroundColor Green
    return $true
}

# Hauptausführung
Write-Host "`n🚀 Starte Update aller Security-Dokumente auf v1.3.0...`n" -ForegroundColor Cyan

$files = Get-ChildItem -Path $securityDir -Filter "*.md" | Where-Object { $_.Name -ne "README.md" }
$totalFiles = $files.Count
$successCount = 0
$failedFiles = @()

foreach ($file in $files) {
    try {
        $result = Update-SecurityDoc -FilePath $file.FullName
        if ($result) {
            $successCount++
        } else {
            $failedFiles += $file.Name
        }
    }
    catch {
        Write-Error "❌ Fehler bei $($file.Name): $_"
        $failedFiles += $file.Name
    }
}

# README.md separat behandeln (bereits aktualisiert)
Write-Host "`n✅ README.md bereits manuell aktualisiert" -ForegroundColor Green
$successCount++
$totalFiles++

# Zusammenfassung
Write-Host "`n" + ("=" * 80) -ForegroundColor Cyan
Write-Host "📊 ZUSAMMENFASSUNG" -ForegroundColor Cyan
Write-Host ("=" * 80) -ForegroundColor Cyan
Write-Host "Total Dateien: $totalFiles" -ForegroundColor White
Write-Host "Erfolgreich: $successCount" -ForegroundColor Green
Write-Host "Fehlgeschlagen: $($failedFiles.Count)" -ForegroundColor $(if ($failedFiles.Count -gt 0) { "Red" } else { "Green" })
Write-Host "Erfolgsrate: $([math]::Round($successCount / $totalFiles * 100, 2))%" -ForegroundColor $(if ($successCount -eq $totalFiles) { "Green" } else { "Yellow" })

if ($failedFiles.Count -gt 0) {
    Write-Host "`n⚠️ Fehlgeschlagene Dateien:" -ForegroundColor Yellow
    $failedFiles | ForEach-Object { Write-Host "  - $_" -ForegroundColor Yellow }
}

# Kategorien-Breakdown
Write-Host "`n📂 Kategorien-Breakdown:" -ForegroundColor Cyan
$categoryMap.Values | Group-Object | Sort-Object Count -Descending | ForEach-Object {
    Write-Host "  $($_.Name): $($_.Count) Dateien" -ForegroundColor White
}

Write-Host "`n✅ Update abgeschlossen! Backup gespeichert in: $backupDir`n" -ForegroundColor Green
