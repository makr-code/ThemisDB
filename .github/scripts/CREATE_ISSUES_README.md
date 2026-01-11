# GitHub Issues Generator from Templates

Ein Python-Script zum automatischen Erstellen von GitHub Issues aus Markdown-Templates und optional zum Löschen der Templates nach Erstellung.

## Features

- 📋 **Template Parsing**: Extrahiert Titel, Labels und Body aus Markdown-Frontmatter
- 🔍 **Duplikat-Erkennung**: Prüft, ob Issues bereits auf GitHub existieren
- ⏭️ **Smart Skipping**: Überspringt bereits erstellte Issues automatisch
- 🗑️ **Auto-Cleanup**: Optional: Löscht Templates nach erfolgreicher Erstellung
- 🧪 **Dry-Run Mode**: Testet ohne echte Änderungen zu machen
- 📊 **Detaillierte Reports**: Zeigt Status aller verarbeiteten Templates

## Installation

```bash
# Clone repository (falls noch nicht geschehen)
cd /path/to/themisdb

# Script ist bereits vorhanden:
python create_issues_from_templates.py
```

## Verwendung

### Basis-Verwendung: Templates anzeigen ohne Änderungen

```bash
python create_issues_from_templates.py --dry-run
```

### Issues erstellen (ohne Templates zu löschen)

```bash
python create_issues_from_templates.py
```

### Issues erstellen UND Templates löschen

```bash
python create_issues_from_templates.py --delete-templates
```

### Dry-Run mit Template-Löschung testen

```bash
python create_issues_from_templates.py --dry-run --delete-templates
```

### Custom Repository-Root

```bash
python create_issues_from_templates.py --repo-root /path/to/repo --delete-templates
```

## Template-Format

Templates müssen sich in `.github/ISSUE_TEMPLATE/` befinden und diesem Format entsprechen:

```markdown
---
name: Issue Name
about: Kurze Beschreibung
title: '[LABEL] Issue Title'
labels: 'type:enhancement, area:storage, priority:P0'
assignees: ''
---

# Issue Title

## 📋 Summary
...
```

### Frontmatter-Felder

| Feld | Format | Beispiel |
|------|--------|----------|
| `name` | String | `"LoRA Framework Implementation"` |
| `about` | String | `"Implement comprehensive LoRA framework"` |
| `title` | String | `"[FEATURE] LoRA Framework"` |
| `labels` | Komma-getrennt oder Array | `'type:enhancement, area:llm'` oder `['type:enhancement', 'area:llm']` |
| `assignees` | Array | `['user1', 'user2']` oder `''` |

## Output

### Success

```
🚀 GitHub Issues Generator from Templates
------------------------------------------------------------
------------------------------------------------------------

Checking existing issues on GitHub...
✓ Found 15 existing issues on GitHub

Found 14 template(s)

Processing: git_features_phase1_named_snapshots.md
  ⏭️  Skipped: Issue already exists

Processing: new_feature.md
  ✅ Created: #342

...

============================================================
SUMMARY
============================================================
✅ Issues Created:       1
⏭️  Issues Skipped:      13
❌ Issues Failed:        0
🗑️  Templates Deleted:  1

Skipped Issues (already exist on GitHub):
  - [FEATURE] Phase 1: Named Snapshots

Created Issues:
  - [FEATURE] New Feature

============================================================

✅ All 13 issue(s) already exist on GitHub
```

## Exit Codes

| Code | Bedeutung |
|------|-----------|
| 0 | ✅ Erfolgreich (alle Issues erstellt oder skipped) |
| 1 | ❌ Fehler (Issues fehlgeschlagen oder keine Aktionen) |

## Workflow

Das Script folgt diesem Workflow für jedes Template:

1. 📄 **Parse Template**: Liest Frontmatter aus Markdown
2. 🔍 **Check Existing**: Vergleicht Titel mit existierenden GitHub Issues
3. ⏭️ **Skip oder Create**: 
   - Falls Issue existiert → Skipped
   - Falls neu → Erstellt Issue
4. 🗑️ **Optional Cleanup**: Löscht Template wenn `--delete-templates`
5. 📊 **Report**: Zeigt Zusammenfassung

## Fehlerbehandlung

### Template-Parsing Fehler

```
⚠️  No frontmatter found in template.md
```

**Lösung**: Template muss `---` zu Anfang und Ende des Frontmatters haben

### GitHub CLI Fehler

```
❌ Failed: fatal: not a git repository
```

**Lösung**: Script muss im Repository-Root ausgeführt werden

### Duplicate Labels

```
⚠️  Could not parse existing issues
```

**Lösung**: GitHub CLI muss installiert und authentifiziert sein (`gh auth status`)

## Best Practices

1. ✅ **Immer Dry-Run machen**: Vor `--delete-templates` erst `--dry-run` testen
2. ✅ **Regelmäßig updaten**: Templates sind Source of Truth für Issues
3. ✅ **Versionskontrolle**: Templates in Git committen vor Löschung
4. ✅ **Labels validieren**: Stellen Sie sicher, dass Labels auf GitHub existieren

## Beispiel-Workflow

```bash
# 1. Neue Templates in .github/ISSUE_TEMPLATE/ hinzufügen
# 2. Testen ohne Änderungen
python create_issues_from_templates.py --dry-run

# 3. Review der Output
# 4. Bei Bedarf Templates anpassen
# 5. Echte Erstellung
python create_issues_from_templates.py

# 6. Wenn erfolgreich, Templates löschen (optional)
python create_issues_from_templates.py --delete-templates
```

## CI/CD Integration

### GitHub Actions Workflow Beispiel

```yaml
name: Create Issues from Templates

on:
  push:
    paths:
      - '.github/ISSUE_TEMPLATE/*.md'
    branches:
      - develop

jobs:
  create-issues:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.11'
      
      - name: Create GitHub Issues
        env:
          GH_TOKEN: ${{ secrets.GITHUB_TOKEN }}
        run: |
          python create_issues_from_templates.py
```

## Troubleshooting

### "No templates found"

```bash
# Prüfen ob Verzeichnis existiert
ls -la .github/ISSUE_TEMPLATE/

# Prüfen ob Dateien .md Endung haben
find .github/ISSUE_TEMPLATE/ -type f
```

### "Could not fetch existing issues"

```bash
# GitHub CLI Status prüfen
gh auth status

# Authentifizierung neu starten
gh auth login
```

### "Issue creation failed"

```bash
# Einzelnes Template manuell testen
gh issue create --title "Test" --body "Test" --label "type:enhancement"
```

## Entwicklung

Das Script nutzt:
- **Subprocess**: Für `gh` CLI Aufrufe
- **Regex**: Für Frontmatter Parsing
- **pathlib**: Für Dateisystem-Operationen
- **argparse**: Für CLI-Argumente

Struktur:
- `IssueTemplateProcessor`: Hauptklasse
  - `parse_template()`: Frontmatter-Extraction
  - `get_existing_issues()`: GitHub-Abfrage
  - `create_issue()`: Issue-Erstellung
  - `delete_template()`: Template-Löschung
  - `process_all_templates()`: Hauptlogik

## Lizenz

Siehe [LICENSE](../../LICENSE)
