# Füge fehlende Sections zu allen 8 Feature-Dateien hinzu

$docsPath = "C:\VCC\themis\docs\de\features"
$files = @(
    'features_extended_compliance.md',
    'features_government_network.md',
    'features_governance_usage.md',
    'features_multi_tenancy.md',
    'features_priorities.md',
    'features_transactions.md'
)

$defaultEnd = @"

## 💡 Best Practices

| ✅ Empfohlen | ❌ Vermeiden |
|--------------|--------------|
| Dokumentierte Best Practices | Anti-Patterns ignorieren |
| Regelmäßiges Testing | Deployment ohne Tests |
| Monitoring aktivieren | Blind Deployments |

## 🔧 Troubleshooting

<details>
<summary><b>Häufige Probleme</b></summary>

Siehe Logs für Details.

</details>

## 📚 Siehe auch

- [Security Best Practices](../security/best_practices.md)
- [Architecture Guide](../architecture/overview.md)

## 📝 Changelog

| Version | Datum | Änderungen |
|---------|-------|-----------|
| v1.3.0 | 2025-12-22 | Template-Aktualisierung für v1.3.0 Standard |

---

**Letzte Aktualisierung:** 22. Dezember 2025  
**Autor:** ThemisDB Team  
**Status:** ✅ Produktiv
"@

foreach ($file in $files) {
    $path = Join-Path $docsPath $file
    if (Test-Path $path) {
        $content = Get-Content $path -Raw
        
        # Check if already has Best Practices
        if ($content -notlike "*## 💡 Best Practices*") {
            # Add sections
            $updatedContent = $content + $defaultEnd
            Set-Content $path $updatedContent -NoNewline
            Write-Host "✅ Updated: $file"
        } else {
            Write-Host "⏭️ Skipped: $file (already has sections)"
        }
    }
}
