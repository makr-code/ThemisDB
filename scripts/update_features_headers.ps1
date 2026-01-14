# PowerShell script to add headers to remaining feature files

$featuresToUpdate = @(
    @{
        file = "features_extended_compliance.md"
        category = "🛡️ Security/Compliance"
        emoji = "⚖️"
        desc = "Zusätzliche Compliance-Features und Implementierungsroadmap."
    },
    @{
        file = "features_governance_usage.md"
        category = "🛡️ Security/Compliance"
        emoji = "🏢"
        desc = "Governance Policy Engine Usage Examples und Best Practices."
    },
    @{
        file = "features_government_network.md"
        category = "🛡️ Security/Compliance"
        emoji = "🇺🇸"
        desc = "Government-Netzwerk Integration und Institutional Network Model."
    },
    @{
        file = "features_multi_tenancy.md"
        category = "⚙️ Infrastructure"
        emoji = "🏢"
        desc = "Sichere Mehrmieter-Isolation und Multi-Tenant Support."
    },
    @{
        file = "features_transactions.md"
        category = "⚙️ Infrastructure"
        emoji = "💳"
        desc = "ACID-Transaktionen mit MVCC für Datenkonsistenz."
    },
    @{
        file = "features_priorities.md"
        category = "⚙️ Infrastructure"
        emoji = "⭐"
        desc = "Query Priority & QoS Management für Performance."
    },
    @{
        file = "features_enterprise_ingestion.md"
        category = "🔄 Data Operations"
        emoji = "📥"
        desc = "Batch & Real-Time Daten-Import für Enterprise-Szenarien."
    }
)

$basePath = "C:\VCC\themis\docs\de\features"

foreach($item in $featuresToUpdate) {
    $fullPath = "$basePath\$($item.file)"
    
    if(Test-Path $fullPath) {
        Write-Host "Updating $($item.file)..." -ForegroundColor Cyan
        
        # Read the file
        $content = Get-Content -Path $fullPath -Raw -Encoding UTF8
        
        # Check if file already has the new header format
        if($content -like "---`ncategory:*") {
            Write-Host "  Already updated (has v1.3.0 format)" -ForegroundColor Green
            continue
        }
        
        # Extract the old header (first few lines until ---)
        $lines = $content -split "`n"
        $headerEnd = 0
        for($i = 0; $i -lt $lines.Count; $i++) {
            if($lines[$i] -eq "---" -or $lines[$i].StartsWith("**Stand:") -or $lines[$i].StartsWith("**Version:") -or $lines[$i].StartsWith("**Status:")) {
                $headerEnd = $i + 1
                break
            }
            if($lines[$i].StartsWith("# ")) {
                # First heading found, insert before it
                $headerEnd = $i
                break
            }
        }
        
        # Build new header
        $newHeader = @"
---
category: "$($item.category)"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# $($item.emoji) $($lines[0] -replace '^# ', '')

$($item.desc)

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features](#-features)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

## 📋 Übersicht

"@
        
        # Get the rest of the content after the old header
        $remainingContent = if($headerEnd -lt $lines.Count) {
            ($lines | Select-Object -Skip $headerEnd) -join "`n"
        } else {
            ""
        }
        
        # Combine and save
        $newContent = $newHeader + "`n" + $remainingContent
        Set-Content -Path $fullPath -Value $newContent -Encoding UTF8 -NoNewline
        
        Write-Host "  ✅ Updated" -ForegroundColor Green
    } else {
        Write-Host "⚠️ File not found: $fullPath" -ForegroundColor Yellow
    }
}

Write-Host "`nDone!" -ForegroundColor Green
